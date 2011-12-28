/// \file bbsecondary.c Contains code for enabling and disabling the secondary GPU.

/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Joaquín Ignacio Aramendía <samsagax@gmail.com>
 * Author: Jaron Viëtor AKA "Thulinma" <jaron@vietors.com>
 *
 * This file is part of Bumblebee.
 *
 * Bumblebee is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bumblebee is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bumblebee. If not, see <http://www.gnu.org/licenses/>.
 */

#include "bbsecondary.h"
#include "switch/switching.h"
#include "bbrun.h"
#include "bblogger.h"
#include "bbconfig.h"
#include "pci.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

/* PCI Bus ID of the discrete video card, -1 means invalid */
int pci_bus_id = -1;

static int unload_module(char *module_name);

/// Returns 1 if the named module (or a module containing the
/// string in its name) is loaded, 0 otherwise.
/// Returns negative upon error.
/// Works by checking /proc/modules
static int module_is_loaded(char * mod) {
  // use the same buffer length as lsmod
  char buffer[4096];
  FILE * bbs = fopen("/proc/modules", "r");
  int ret = 0;
  // assume mod_len <= sizeof(buffer)
  int mod_len = strlen(mod);

  if (bbs == 0) {//error opening, return -1
    bb_log(LOG_DEBUG, "Couldn't open /proc/modules");
    return -1;
  }
  while (fgets(buffer, sizeof(buffer), bbs)) {
    // match "module" with "module " and not "module-blah"
    if (!strncmp(buffer, mod, mod_len) && isspace(buffer[mod_len])) {
      // module is found
      ret = 1;
      break;
    }
  }
  fclose(bbs);
  return ret;
}//module_is_loaded

/// Start the X server by fork-exec, turn card on and load driver if needed.
/// If after this method finishes X is running, it was successfull.
/// If it somehow fails, X should not be running after this method finishes.
void start_secondary(void) {
  char driver[BUFFER_SIZE] = {0};
  /* enable card if the switcher is available */
  if (switcher) {
    if (switch_on() != SWITCH_ON) {
      set_bb_error("Could not enable discrete graphics card");
      return;
    }
  }

  //if runmode is BB_RUN_EXIT, do not start X, we are shutting down.
  if (bb_status.runmode == BB_RUN_EXIT) {
    return;
  }

  if (pci_get_driver(driver, pci_bus_id, sizeof driver)) {
    /* if the loaded driver does not equal the driver from config, unload it */
    if (strcasecmp(bb_config.driver, driver)) {
      if (!unload_module(driver)) {
        /* driver failed to unload, aborting */
        return;
      }
    }
  }

  /* load the driver if none was loaded or if the loaded driver did not match
   * the configured one */
  if (strcasecmp(bb_config.driver, driver)) {
    bb_log(LOG_INFO, "Loading %s module\n", bb_config.driver);
    char * mod_argv[] = {
      "modprobe",
      *bb_config.module_name ? bb_config.module_name : bb_config.driver,
      NULL
    };
    bb_run_fork_wait(mod_argv, 10);
  }

  // if driver load failed, cancel and set error
  if (!pci_get_driver(NULL, pci_bus_id, 0)) {
    set_bb_error("Could not load GPU driver");
    return;
  }

  //no problems, start X if not started yet
  if (!bb_is_running(bb_status.x_pid)) {
    bb_log(LOG_INFO, "Starting X server on display %s.\n", bb_config.x_display);
    char pci_id[12];
    snprintf(pci_id, 12, "PCI:%02x:%02x:%o", pci_bus_id >> 8, (pci_bus_id >> 3) & 0x1f, pci_bus_id & 0x7);
    char * x_argv[] = {
      "X",
      bb_config.x_display,
      "-config", bb_config.x_conf_file,
      "-sharevts",
      "-nolisten", "tcp",
      "-noreset",
      "-isolateDevice", pci_id,
      "-modulepath",
      bb_config.mod_path,
      NULL
    };
    bb_status.x_pid = bb_run_fork_ld(x_argv, bb_config.ld_path);
  }

  //check if X is available, for maximum 10 seconds.
  time_t xtimer = time(0);
  Display * xdisp = 0;
  while ((time(0) - xtimer <= 10) && bb_is_running(bb_status.x_pid)) {
    xdisp = XOpenDisplay(bb_config.x_display);
    if (xdisp != 0) {
      break;
    }
    usleep(100000); //don't retry too fast
  }

  //check if X is available
  if (xdisp == 0) {
    //X not available
    /// \todo Maybe check X exit status and/or messages?
    if (bb_is_running(bb_status.x_pid)) {
      //X active, but not accepting connections
      set_bb_error("X unresponsive after 10 seconds - aborting");
      bb_stop(bb_status.x_pid);
    } else {
      //X terminated itself
      set_bb_error("X did not start properly");
    }
  } else {
    //X accepted the connetion - we assume it works
    XCloseDisplay(xdisp); //close connection to X again
    bb_log(LOG_INFO, "X successfully started in %i seconds\n", time(0) - xtimer);
    //reset errors, if any
    set_bb_error(0);
  }
}//start_secondary

/**
 * Attempts to unload a module if loaded, for ten seconds before
 * giving up
 *
 * @param module_name The name of the kernel module to be unloaded
 * @return 1 if the driver is succesfully unloaded, 0 otherwise
 */
static int unload_module(char *module_name) {
  if (module_is_loaded(module_name) == 1) {
    bb_log(LOG_INFO, "Unloading %s module\n", module_name);
    char * mod_argv[] = {
      "rmmod",
      "--wait",
      module_name,
      NULL
    };
    bb_run_fork_wait(mod_argv, 10);
    if (module_is_loaded(module_name) == 1) {
      bb_log(LOG_ERR, "Unloading %s module timed out.\n", module_name);
    }
  }
  return 1;
}//unload module

/// Kill the second X server if any, turn card off if requested.
void stop_secondary() {
  char driver[BUFFER_SIZE];
  // kill X if it is running
  if (bb_is_running(bb_status.x_pid)) {
    bb_log(LOG_INFO, "Stopping X server\n");
    bb_stop_wait(bb_status.x_pid);
  }

  if (!bb_config.pm_enabled && (bb_status.runmode != BB_RUN_EXIT)) {
    return; //do not switch card off if pm_enabled is false, unless exiting.
  }

  //if card is on and can be switched, switch it off
  if (switcher) {
    if (switcher->need_driver_unloaded) {
      /* do not unload the drivers nor disable the card if the card is not on */
      if (switcher->status() != SWITCH_ON) {
        return;
      }
      /* unload the driver loaded by the graphica card */
      if (pci_get_driver(driver,pci_bus_id, sizeof driver)) {
        unload_module(driver);
      }

      //only turn card off if no drivers are loaded
      if (pci_get_driver(NULL, pci_bus_id, 0)) {
        bb_log(LOG_DEBUG, "Drivers are still loaded, unable to disable card\n");
        return;
      }
    }
    if (switch_off() != SWITCH_OFF) {
      bb_log(LOG_WARNING, "Unable to disable discrete card.");
    }
  }
}//stop_secondary

/// Returns 0 if card is off, 1 if card is on, -1 if not-switchable.
int status_secondary(void) {
  switch (switch_status()) {
    case SWITCH_ON:
      return 1;
    case SWITCH_OFF:
      return 0;
    case SWITCH_UNAVAIL:
    default:
      return -1;
  }
}

/// Checks what methods are available and what drivers are installed.
/// Sets sane defaults for the current environment, also prints
/// debug messages including the found hardware/software.
/// Will print warning message if no switching method is found.
void check_secondary(void) {
  //check installed drivers
  if (*bb_config.driver) {
    bb_log(LOG_DEBUG, "Skipping auto-detection, using configured driver"
            " '%s'\n", bb_config.driver);
  } else if (module_is_loaded("nvidia")) {
    snprintf(bb_config.driver, BUFFER_SIZE, "nvidia");
    bb_log(LOG_DEBUG, "Detected nvidia driver\n");
  } else if (module_is_loaded("nouveau")) {
    snprintf(bb_config.driver, BUFFER_SIZE, "nouveau");
    bb_log(LOG_DEBUG, "Detected nouveau driver\n");
  } else {
    strncpy(bb_config.driver, CONF_DRIVER, BUFFER_SIZE);
    bb_log(LOG_WARNING, "No driver auto-detected. Using compile default '%s'"
            " instead.\n", bb_config.driver);
  }

  //check switch availability, warn if not availble
  switcher = switcher_detect(NULL);
  if (switcher) {
    bb_log(LOG_INFO, "Switching method '%s' is available and will be used.\n",
            switcher->name);
  } else {
    bb_log(LOG_WARNING, "No switching method available. The dedicated card will"
            " always be on.\n");
  }
}
