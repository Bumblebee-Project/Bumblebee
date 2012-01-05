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

#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include "bbsecondary.h"
#include "switch/switching.h"
#include "bbrun.h"
#include "bblogger.h"
#include "bbconfig.h"
#include "pci.h"
#include "module.h"

/* PCI Bus ID of the discrete video card, -1 means invalid */
int pci_bus_id = -1;

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
      if (!module_unload(driver)) {
        /* driver failed to unload, aborting */
        return;
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
      return;
    }
  }

  //no problems, start X if not started yet
  if (!bb_is_running(bb_status.x_pid)) {
    char pci_id[12];
    static char *x_conf_file;
    snprintf(pci_id, 12, "PCI:%02x:%02x:%o", pci_bus_id >> 8, (pci_bus_id >> 3)
            & 0x1f, pci_bus_id & 0x7);
    if (!x_conf_file) {
      x_conf_file = xorg_path_w_driver(bb_config.x_conf_file, bb_config.driver);
    }

    bb_log(LOG_INFO, "Starting X server on display %s.\n", bb_config.x_display);
    char *x_argv[] = {
      XORG_BINARY,
      bb_config.x_display,
      "-config", x_conf_file,
      "-sharevts",
      "-nolisten", "tcp",
      "-noreset",
      "-isolateDevice", pci_id,
      "-modulepath",
      bb_config.mod_path,
      NULL
    };
    if (!*bb_config.mod_path) {
      x_argv[10] = 0;//remove -modulepath if not set
    }
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

/// Kill the second X server if any, turn card off if requested.
void stop_secondary() {
  char driver[BUFFER_SIZE];
  // kill X if it is running
  if (bb_is_running(bb_status.x_pid)) {
    bb_log(LOG_INFO, "Stopping X server\n");
    bb_stop_wait(bb_status.x_pid);
  }

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
      if (pci_get_driver(driver,pci_bus_id, sizeof driver)) {
        module_unload(driver);
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
  /* determine driver to be used */
  if (*bb_config.driver) {
    bb_log(LOG_DEBUG, "Skipping auto-detection, using configured driver"
            " '%s'\n", bb_config.driver);
  } else if (strlen(CONF_DRIVER)) {
    /* if the default driver is set, use that */
    bb_log(LOG_DEBUG, "Using compile default driver '%s'", CONF_DRIVER);
  } else if (module_is_loaded("nouveau")) {
    /* loaded drivers take precedence over ones available for modprobing */
    set_string_value(&bb_config.driver, "nouveau");
    set_string_value(&bb_config.module_name, "nouveau");
    bb_log(LOG_DEBUG, "Detected nouveau driver\n");
  } else if (module_is_available("nvidia") ||
          module_is_available("nvidia-current")) {
    /* Ubuntu and Mandriva use nvidia-current.ko. nvidia cannot be compiled into
     * the kernel, so module_is_loaded makes module_is_available redundant */
    set_string_value(&bb_config.driver, "nvidia");
    bb_log(LOG_DEBUG, "Detected nvidia driver\n");
  } else if (module_is_available("nouveau")) {
    set_string_value(&bb_config.driver, "nouveau");
    set_string_value(&bb_config.module_name, "nouveau");
    bb_log(LOG_DEBUG, "Detected nouveau driver\n");
  }

  if (!*bb_config.module_name) {
    /* no module has been configured, set a sensible one based on driver */
    if (strcmp(bb_config.driver, "nvidia") == 0 &&
            module_is_available("nvidia-current")) {
      set_string_value(&bb_config.module_name, "nvidia-current");
    } else {
      set_string_value(&bb_config.module_name, bb_config.driver);
    }
  }

  //check switch availability, warn if not availble
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
