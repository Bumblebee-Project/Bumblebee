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
#include "bbswitch.h"
#include "bbrun.h"
#include "bblogger.h"
#include "bbconfig.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


/// Returns 1 if the named module (or a module containing the
/// string in its name) is loaded, 0 otherwise.
/// Returns negative upon error.
/// Works by checking /proc/modules
static int module_is_loaded(char * mod) {
  char buffer[BUFFER_SIZE];
  char * r = buffer;
  FILE * bbs = fopen("/proc/modules", "r");
  if (bbs == 0){return -1;}//error opening, return -1
  while (r != 0) {
    r = fgets(buffer, BUFFER_SIZE - 1, bbs);
    if (strstr(buffer, mod)){
      //found module, return 1
      fclose(bbs);
      return 1;
    }
  }
  //not found, return 0
  fclose(bbs);
  return 0;
}//module_is_loaded

/// Checks if either nvidia or nouveau is currently loaded.
/// Returns 1 if yes, 0 otherwise (not loaded / error occured).
static int is_driver_loaded() {
  if (module_is_loaded("nvidia") == 1) {return 1;}
  if (module_is_loaded("nouveau") == 1) {return 1;}
  return 0;
}//is_driver_loaded

/// Sets error message for problems starting the secondary.
/// Sets bb_status.errors as well as printing the error.
static void set_secondary_error(char * msg) {
  snprintf(bb_status.errors, BUFFER_SIZE, "%s", msg);
  bb_log(LOG_ERR, "%s\n", msg);
}//set_secondary_error


/// Start the X server by fork-exec, turn card on and load driver if needed.
/// If after this method finishes X is running, it was successfull.
/// If it somehow fails, X should not be running after this method finishes.
void start_secondary(void) {
  //if card can be switch and is off, turn it on
  if (bbswitch_status() == 0) {
    bb_log(LOG_INFO, "Switching dedicated card ON\n");
    bbswitch_on();
  }

  // if card switch failed, cancel and set error
  if (bbswitch_status() == 0) {
    set_secondary_error("Could not switch dedicated card on");
    return;
  }

  /// \todo Support nouveau as well
  if (!is_driver_loaded()) {//only load if not already loaded
    bb_log(LOG_INFO, "Loading nvidia module\n");
    char * mod_argv[] = {
      "modprobe",
      "nvidia",
      NULL
    };
    bb_run_fork_wait(mod_argv);
  }

  // if driver load failed, cancel and set error
  if (!is_driver_loaded()) {
    set_secondary_error("Could not load GPU driver");
    return;
  }

  //no problems, start X
  bb_log(LOG_INFO, "Starting X server on display %s.\n", bb_config.x_display);
  char * x_argv[] = {
    "X",
    "-config", bb_config.x_conf_file,
    "-sharevts",
    "-nolisten", "tcp",
    "-noreset",
    bb_config.x_display,
    NULL
  };
  bb_status.x_pid = bb_run_fork_ld(x_argv, bb_config.ld_path);

  time_t xtimer = time(0);
  Display * xdisp = 0;
  //wait for X to become available for a maximum of 10 seconds
  while ((time(0) - xtimer <= 10) && bb_is_running(bb_status.x_pid)) {
    xdisp = XOpenDisplay(bb_config.x_display);
    if (xdisp != 0) {
      break;
    }
    usleep(100000);//don't retry too fast
  }

  //check if X is available
  if (xdisp == 0) {
    //X not available
    /// \todo Maybe check X exit status and/or messages?
    if (bb_is_running(bb_status.x_pid)) {
      //X active, but not accepting connections
      set_secondary_error("X unresponsive after 10 seconds - aborting");
      bb_stop(bb_status.x_pid);
    } else {
      //X terminated itself
      set_secondary_error("X did not start properly");
    }
  } else {
    //X accepted the connetion - we assume it works
    XCloseDisplay(xdisp); //close connection to X again
    bb_log(LOG_INFO, "X successfully started in %i seconds\n", time(0) - xtimer);
  }
}//start_secondary

/// Kill the second X server if any, turn card off if requested.
void stop_secondary(void) {
  // make repeated attempts to kill X
  /// \todo Perhaps switch to a different signal after a few tries? At least put a timeout in...
  while (bb_is_running(bb_status.x_pid)) {
    bb_log(LOG_INFO, "Stopping X server\n");
    bb_stop(bb_status.x_pid);
    //usleep returns if interrupted by a signal (child process death)
    usleep(5000000);//sleep for max 5 seconds
  }

  //if card is on and can be switched, switch it off
  if (bbswitch_status() == 1) {
    //unload nvidia driver first, if loaded
    /// \todo Support nouveau as well
    if (module_is_loaded("nvidia")) {
      bb_log(LOG_INFO, "Unloading nvidia module\n");
      char * mod_argv[] = {
        "rmmod",
        "nvidia",
        NULL
      };
      bb_run_fork_wait(mod_argv);
    }

    //only turn card off if no drivers are loaded
    if (!is_driver_loaded()) {
      bb_log(LOG_INFO, "Switching dedicated card OFF\n");
      bbswitch_off();
    } else {
      bb_log(LOG_DEBUG, "Delaying card OFF - drivers are still loaded\n");
    }
  }
}//stop_secondary
