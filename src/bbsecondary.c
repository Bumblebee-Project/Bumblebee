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

#define _GNU_SOURCE
#include <unistd.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "bbsecondary.h"
#include "switch/switching.h"
#include "bbrun.h"
#include "bblogger.h"
#include "bbconfig.h"
#include "pci.h"
#include "module.h"

/**
 * Substitutes DRIVER in the passed path
 * @param x_conf_file A path to be processed
 * @param driver The replacement for each occurence of DRIVER
 * @return A path to xorg.conf with DRIVER substituted for the driver
 */
static char *xorg_path_w_driver(char *x_conf_file, char *driver) {
  static char *path;
  const char *driver_keyword = "DRIVER";
  unsigned int driver_occurences = 0;
  int path_length;
  char *pos, *next;

  /* calculate the path buffer size */
  pos = x_conf_file;
  while ((next = strstr(pos, driver_keyword)) != 0) {
    driver_occurences++;
    pos = next + strlen(driver_keyword);
  }
  path_length = strlen(x_conf_file) +
          driver_occurences * (strlen(driver_keyword) - 1);

  /* allocate some memory including null byte and make it an empty string */
  path = malloc(path_length + 1);
  if (!path) {
    bb_log(LOG_WARNING, "Could not allocate memory for xorg conf path\n");
    return NULL;
  }
  path[0] = 0;

  /* now replace for real */
  pos = x_conf_file;
  while ((next = strstr(pos, driver_keyword)) != 0) {
    int len = next - pos;
    strncat(path, pos, len);
    strncat(path, driver, path_length);

    /* the next search starts at the position after %s */
    pos = next + strlen(driver_keyword);
  }
  /* append the remainder after the last %s if any and overwrite the setting */
  strncat(path, pos, path_length);
  return path;
}

/**
 * Load the kernel module, powering on the card beforehand
 */
static bool switch_and_load(void)
{
  char driver[BUFFER_SIZE] = {0};
  /* enable card if the switcher is available */
  if (switcher) {
    if (switch_on() != SWITCH_ON) {
      set_bb_error("Could not enable discrete graphics card");
      return false;
    }
  }

  //if runmode is BB_RUN_EXIT, do not start X, we are shutting down.
  if (bb_status.runmode == BB_RUN_EXIT) {
    return false;
  }

  if (pci_get_driver(driver, pci_bus_id_discrete, sizeof driver)) {
    /* if the loaded driver does not equal the driver from config, unload it */
    if (strcasecmp(bb_config.driver, driver)) {
      if (!module_unload(driver)) {
        /* driver failed to unload, aborting */
        return false;
      }
    }
  }

  /* load the driver if none was loaded or if the loaded driver did not match
   * the configured one */
  if (strcasecmp(bb_config.driver, driver)) {
    char *module_name = bb_config.module_name;
    char *driver_name = bb_config.driver;
    if (!module_load(module_name, driver_name)) {
      set_bb_error("Could not load GPU driver");
      return false;
    }
  }
  return true;
}

/**
 * Start the X server by fork-exec, turn card on and load driver if needed.
 * If after this method finishes X is running, it was successfull.
 * If it somehow fails, X should not be running after this method finishes.
 */
bool start_secondary(bool need_secondary) {
  if (!switch_and_load())
    return false;
  if (!need_secondary)
    return true;
  //no problems, start X if not started yet
  if (!bb_is_running(bb_status.x_pid)) {
    char pci_id[12];
    static char *x_conf_file;
    snprintf(pci_id, 12, "PCI:%02x:%02x:%o", pci_bus_id_discrete->bus,
            pci_bus_id_discrete->slot, pci_bus_id_discrete->func);
    if (!x_conf_file) {
      x_conf_file = xorg_path_w_driver(bb_config.x_conf_file, bb_config.driver);
    }

    bb_log(LOG_INFO, "Starting X server on display %s.\n", bb_config.x_display);
    char *x_argv[] = {
      XORG_BINARY,
      bb_config.x_display,
      "-config", x_conf_file,
      "-configdir", bb_config.x_conf_dir,
      "-sharevts",
      "-nolisten", "tcp",
      "-noreset",
      "-verbose", "3",
      "-isolateDevice", pci_id,
      "-modulepath",
      bb_config.mod_path,
      NULL
    };
    if (!*bb_config.mod_path) {
      x_argv[12] = 0; //remove -modulepath if not set
    }
    //close any previous pipe, if it (still) exists
    if (bb_status.x_pipe[0] != -1){close(bb_status.x_pipe[0]); bb_status.x_pipe[0] = -1;}
    if (bb_status.x_pipe[1] != -1){close(bb_status.x_pipe[1]); bb_status.x_pipe[1] = -1;}
    //create a new pipe
    if (pipe2(bb_status.x_pipe, O_NONBLOCK | O_CLOEXEC)){
      set_bb_error("Could not create output pipe for X");
      return false;
    }
    bb_status.x_pid = bb_run_fork_ld_redirect(x_argv, bb_config.ld_path, bb_status.x_pipe[1]);
    //close the end of the pipe that is not ours
    if (bb_status.x_pipe[1] != -1){close(bb_status.x_pipe[1]); bb_status.x_pipe[1] = -1;}
  }

  //check if X is available, for maximum 10 seconds.
  time_t xtimer = time(0);
  Display * xdisp = 0;
  while ((time(0) - xtimer <= 10) && bb_is_running(bb_status.x_pid)) {
    xdisp = XOpenDisplay(bb_config.x_display);
    if (xdisp != 0) {
      break;
    }
    check_xorg_pipe();//make sure Xorg errors come in smoothly
    usleep(100000); //don't retry too fast
  }
  check_xorg_pipe();//make sure Xorg errors come in smoothly

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
    return true;
  }
  return false;
}//start_secondary

/**
 * Unload the kernel module and power down the card
 */
static void switch_and_unload(void)
{
  char driver[BUFFER_SIZE];

  if (bb_config.pm_method == PM_DISABLED && bb_status.runmode != BB_RUN_EXIT) {
    /* do not disable the card if PM is disabled unless exiting */
    return;
  }

  //if card is on and can be switched, switch it off
  if (switcher) {
    if (switcher->need_driver_unloaded) {
      /* do not unload the drivers nor disable the card if the card is not on */
      if (switcher->status() != SWITCH_ON) {
        return;
      }
      /* unload the driver loaded by the graphica card */
      if (pci_get_driver(driver, pci_bus_id_discrete, sizeof driver)) {
        module_unload(driver);
      }

      //only turn card off if no drivers are loaded
      if (pci_get_driver(NULL, pci_bus_id_discrete, 0)) {
        bb_log(LOG_DEBUG, "Drivers are still loaded, unable to disable card\n");
        return;
      }
    }
    if (switch_off() != SWITCH_OFF) {
      bb_log(LOG_WARNING, "Unable to disable discrete card.");
    }
  }
}

/**
 * Kill the second X server if any, turn card off if requested.
 */
void stop_secondary() {
  // kill X if it is running
  if (bb_is_running(bb_status.x_pid)) {
    bb_log(LOG_INFO, "Stopping X server\n");
    bb_stop_wait(bb_status.x_pid);
  }
  switch_and_unload();
}//stop_secondary

/**
 * Check for the availability of a PM method, warn if no method is available
 */
void check_pm_method(void) {
  if (bb_config.pm_method == PM_DISABLED) {
    bb_log(LOG_INFO, "PM is disabled, not performing detection.\n");
  } else {
    struct switch_info info;
    memset(&info, 0, sizeof info);
    info.driver = bb_config.driver;
    info.configured_pm = bb_pm_method_string[bb_config.pm_method];

    const char *pm_method = NULL;
    if (bb_config.pm_method != PM_AUTO) {
      /* auto-detection override */
      pm_method = bb_pm_method_string[bb_config.pm_method];
    }

    switcher = switcher_detect(pm_method, info);
    if (switcher) {
      bb_log(LOG_INFO, "Switching method '%s' is available and will be used.\n",
              switcher->name);
    } else {
      bb_log(LOG_WARNING, "No switching method available. The dedicated card"
              " will always be on.\n");
    }
  }
}
