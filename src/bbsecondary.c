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
#include "bbglobals.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


/// Returns 1 if the named module (or a module containing the
/// string in its name) is loaded, 0 otherwise.
/// Returns negative upon error.
/// Works by checking /proc/modules
static int module_is_loaded(char * mod){
  char buffer[BUFFER_SIZE];
  char * r = buffer;
  FILE * bbs = fopen("/proc/modules", "r");
  if (bbs == 0){return -1;}//error opening, return -1
  while (r != 0){
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
}


/**
 *  Start the X server by fork-exec, turn card on if needed.
 *
 *  @return 0 for success, anything else for failure.
 */
void start_secondary(void) {
  if (bbswitch_status() == 0) {
    bb_log(LOG_INFO, "Switching dedicated card ON\n");
    bbswitch_on();
    /// \todo Support nouveau as well
    if (bbswitch_status() != 0){//only load module if card is on
      bb_log(LOG_INFO, "Loading nvidia module\n");
      char * mod_argv[] = {
        "modprobe",
        "nvidia",
        NULL
      };
      bb_run_fork_wait(mod_argv);
    }
  }
  //only start X if card on and either nvidia or nouveau is loaded
  if (bbswitch_status() != 0 && (module_is_loaded("nvidia") || module_is_loaded("nouveau"))) {
    bb_log(LOG_INFO, "Starting X server on display %s.\n", bb_config.xdisplay);
    char * x_argv[] = {
      "X",
      "-config", bb_config.xconf,
      "-sharevts",
      "-nolisten", "tcp",
      "-noreset",
      bb_config.xdisplay,
      NULL
    };
    bb_status.x_pid = bb_run_fork(x_argv);
    time_t xtimer = time(0);
    Display * xdisp = 0;
    while ((time(0) - xtimer <= 10) && bb_is_running(bb_status.x_pid)) {
      xdisp = XOpenDisplay(bb_config.xdisplay);
      if (xdisp != 0) {
        break;
      }
    }
    if (xdisp == 0) {
      /// \todo Maybe check X exit status and/or messages?
      if (bb_is_running(bb_status.x_pid)) {
        bb_log(LOG_ERR, "X unresponsive after 10 seconds - aborting\n");
        bb_stop(bb_status.x_pid);
        snprintf(bb_status.errors, BUFFER_SIZE, "X unresponsive after 10 seconds - aborting");
      } else {
        bb_log(LOG_ERR, "X did not start properly\n");
        snprintf(bb_status.errors, BUFFER_SIZE, "X did not start properly");
      }
    } else {
      XCloseDisplay(xdisp); //close connection to X again
      bb_log(LOG_INFO, "X successfully started in %i seconds\n", time(0) - xtimer);
    }
  } else {
    snprintf(bb_status.errors, BUFFER_SIZE, "Could not switch dedicated card on.");
  }
}

/**
 * Kill the second X server if any, turn card off if requested.
 */
void stop_secondary(void) {
  while (bb_is_running(bb_status.x_pid)) {
    bb_log(LOG_INFO, "Stopping X server\n");
    bb_stop(bb_status.x_pid);
    usleep(5000000);//sleep for max 5 seconds, less if X is done earlier
  }
  if (bbswitch_status() == 1) {
    /// \todo Support nouveau as well
    bb_log(LOG_INFO, "Unloading nvidia module\n");
    char * mod_argv[] = {
      "rmmod",
      "nvidia",
      NULL
    };
    bb_run_fork_wait(mod_argv);
    //only turn card off if driver is unloaded
    if (!(module_is_loaded("nvidia") && !module_is_loaded("nouveau"))){
      bb_log(LOG_INFO, "Switching dedicated card OFF\n");
      bbswitch_off();
    }
  }
}
