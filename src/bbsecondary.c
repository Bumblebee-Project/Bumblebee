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
#include "bbrun.h"
#include "bblogger.h"
#include "bbconfig.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BBS_BUFFER 100

/// Returns 0 if card is off, 1 if card is on, -1 if bbswitch not active.
/// In other words: 0 means off, anything else means on.
static int bbswitch_status(void) {
  char buffer[BBS_BUFFER];
  int i, r;
  FILE * bbs = fopen("/proc/acpi/bbswitch", "r");
  if (bbs == 0) {
    bb_log(LOG_DEBUG, "Couldn't open bbswitch FIFO file, bbswitch not active\n");
    return -1;
  }
  for (i = 0; i < BBS_BUFFER; ++i) {
    buffer[i] = 0;
  }
  r = fread(buffer, 1, BBS_BUFFER - 1, bbs);
  fclose(bbs);
  for (i = 0; (i < BBS_BUFFER) && (i < r); ++i) {
    if (buffer[i] == ' ') {//find the space
      if (buffer[i + 2] == 'F') {
        bb_log(LOG_DEBUG, "The discrete card is OFF [bbswitch]\n");
        return 0;
      }//OFF
      if (buffer[i + 2] == 'N') {
        bb_log(LOG_DEBUG, "The discrete card is ON [bbswitch]\n");
        return 1;
      }//ON
    }
    if (buffer[i] == 0) {
      return -1;
    }//end of string, stop search
  }
  return -1; //space not found - assume bbswitch isn't working
}//bbswitch_status

/// Turns card on if not already on.
static void bbswitch_on(void) {
  int r;
  r = bbswitch_status();
  if (r != 0) {
    if (r == 1) {
      bb_log(LOG_INFO, "Card already on, not switching it on [bbswitch]\n");
    } else {
      bb_log(LOG_WARNING, "bbswitch unavailable, not turning dedicated card on\n");
    }
    return;
  }
  FILE * bbs = fopen("/proc/acpi/bbswitch", "w");
  if (bbs == 0) {
    bb_log(LOG_ERR, "Could not access bbswitch module.\n");
    return;
  }
  r = fwrite("ON\n", 1, 4, bbs);
  fclose(bbs);
  if (r < 2) {
    bb_log(LOG_WARNING, "bbswitch isn't listening to us!\n");
  }
  r = bbswitch_status();
  if (r != 1) {
    bb_log(LOG_ERR, "Failed to turn dedicated card on!\n");
  }
}//bbswitch_on

/// Turns card off if not already off.
static void bbswitch_off(void) {
  int r;
  r = bbswitch_status();
  if (r != 1) {
    if (r == 0) {
      bb_log(LOG_INFO, "Card already off, not switching it off [bbswitch]\n");
    } else {
      bb_log(LOG_WARNING, "bbswitch unavailable, not turning dedicated card off.\n");
    }
    return;
  }
  FILE * bbs = fopen("/proc/acpi/bbswitch", "w");
  if (bbs == 0) {
    bb_log(LOG_ERR, "Could not access bbswitch module.\n");
    return;
  }
  r = fwrite("OFF\n", 1, 5, bbs);
  fclose(bbs);
  if (r < 3) {
    bb_log(LOG_WARNING, "bbswitch isn't listening to us!\n");
  }
  r = bbswitch_status();
  if (r != 0) {
    bb_log(LOG_ERR, "Failed to turn dedicated card off!\n");
  }
}//bbswitch_off

/// Returns 0 if card is off, 1 if card is on, -1 if bbswitch not active.
/// In other words: 0 means off, anything else means on.
static switcheroo_status(void) {
  char buffer[BBS_BUFFER];
  char * r = buffer;
  FILE * bbs = fopen("/sys/kernel/debug/vgaswitcheroo/switch", "r");
  if (bbs == 0) {
    return -1;
  }
  while (r != 0){
    r = fgets(buffer, BBS_BUFFER, bbs);
    if (buffer[2] == 'D'){//found the DIS line
      fclose(bbs);
      if (buffer[8] == 'P'){
        bb_log(LOG_DEBUG, "The discrete card is ON [vga_switcheroo]\n");
        return 1;//Pwr
      } else {
        bb_log(LOG_DEBUG, "The discrete card is OFF [vga_switcheroo]\n");
        return 0;//Off
      }
    }
  }
  fclose(bbs);
  return -1; //DIS line not found - assume switcheroo isn't working
}//switcheroo_status

/// Turns card on if not already on.
static void switcheroo_on(void) {
  int r;
  r = switcheroo_status();
  if (r != 0) {
    if (r == 1) {
      bb_log(LOG_INFO, "Card already on, not it on [vga_switcheroo]\n");
    } else {
      bb_log(LOG_WARNING, "vga_switcheroo unavailable, not turning dedicated card on.\n");
    }
    return;
  }
  FILE * bbs = fopen("/sys/kernel/debug/vgaswitcheroo/switch", "w");
  if (bbs == 0) {
    bb_log(LOG_ERR, "Could not access vga_switcheroo.\n");
    return;
  }
  r = fwrite("ON\n", 1, 4, bbs);
  fclose(bbs);
  if (r < 2) {
    bb_log(LOG_WARNING, "vga_switcheroo isn't listening to us!\n");
  }
  r = switcheroo_status();
  if (r != 1) {
    bb_log(LOG_ERR, "Failed to turn dedicated card on!\n");
  }
}//switcheroo_on

/// Turns card off if not already off.
static void switcheroo_off(void) {
  int r;
  r = switcheroo_status();
  if (r != 1) {
    if (r == 0) {
      bb_log(LOG_INFO, "Card already off, not turning it off [vga_switcheroo]\n");
    } else {
      bb_log(LOG_WARNING, "vga_switcheroo unavailable, not turning dedicated card off.\n");
    }
    return;
  }
  FILE * bbs = fopen("/sys/kernel/debug/vgaswitcheroo/switch", "w");
  if (bbs == 0) {
    bb_log(LOG_ERR, "Could not access vga_switcheroo.\n");
    return;
  }
  r = fwrite("OFF\n", 1, 5, bbs);
  fclose(bbs);
  if (r < 2) {
    bb_log(LOG_WARNING, "vga_switcheroo isn't listening to us!\n");
  }
  r = switcheroo_status();
  if (r != 0) {
    bb_log(LOG_ERR, "Failed to turn dedicated card off!\n");
  }
}//switcheroo_off

/// Returns 1 if the named module (or a module containing the
/// string in its name) is loaded, 0 otherwise.
/// Returns negative upon error.
/// Works by checking /proc/modules
static int module_is_loaded(char * mod) {
  char buffer[BUFFER_SIZE];
  char * r = buffer;
  FILE * bbs = fopen("/proc/modules", "r");
  if (bbs == 0) {//error opening, return -1
    bb_log(LOG_DEBUG,"Couldn't open /proc/modules");
    return -1;
  }
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
  int r;
  //if card can be switched by bbswitch and is off, turn it on
  r = bbswitch_status();
  if (r == 0) {
    bb_log(LOG_INFO, "Switching dedicated card ON [bbswitch]\n");
    bbswitch_on();
    // if card switch failed, cancel and set error
    if (bbswitch_status() == 0) {
      set_secondary_error("Could not switch dedicated card on [bbswitch]");
      return;
    }
  }
  //no bbswitch support? attempt vga_switcheroo
  if ((r == -1) && (switcheroo_status() == 0)) {
    bb_log(LOG_INFO, "Switching dedicated card ON [vga_switcheroo]\n");
    switcheroo_on();
    // if card switch failed, cancel and set error
    if (switcheroo_status() == 0) {
      set_secondary_error("Could not switch dedicated card on [vga_switcheroo]");
      return;
    }
  }

  if (!is_driver_loaded()) {//only load if not already loaded
    bb_log(LOG_INFO, "Loading %s module\n", bb_config.driver);
    char * mod_argv[] = {
      "modprobe",
      bb_config.driver,
      NULL
    };
    bb_run_fork_wait(mod_argv);
  }

  // if driver load failed, cancel and set error
  if (!is_driver_loaded()) {
    set_secondary_error("Could not load GPU driver");
    return;
  }

  //no problems, start X if not started yet
  if (!bb_is_running(bb_status.x_pid)) {
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
  }

  //check if X is available, for maximum 10 seconds.
  time_t xtimer = time(0);
  Display * xdisp = 0;
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
void stop_secondary(int switch_card) {
  // make repeated attempts to kill X
  /// \todo Perhaps switch to a different signal after a few tries? At least put a timeout in...
  while (bb_is_running(bb_status.x_pid)) {
    bb_log(LOG_INFO, "Stopping X server\n");
    bb_stop(bb_status.x_pid);
    //usleep returns if interrupted by a signal (child process death)
    usleep(5000000);//sleep for max 5 seconds
  }

  if (!(bb_config.pm_enabled && switch_card)) {
    return;//do not switch card off if pm_enabled is false
  }

  //if card is on and can be switched, switch it off
  if (bbswitch_status() == 1) {
    //unload driver first, if loaded
    if (module_is_loaded(bb_config.driver)) {
      bb_log(LOG_INFO, "Unloading %s module\n", bb_config.driver);
      char * mod_argv[] = {
        "rmmod",
        bb_config.driver,
        NULL
      };
      bb_run_fork_wait(mod_argv);
    }

    //only turn card off if no drivers are loaded
    if (!is_driver_loaded()) {
      bb_log(LOG_INFO, "Switching dedicated card OFF [bbswitch]\n");
      bbswitch_off();
    } else {
      bb_log(LOG_DEBUG, "Delaying card OFF - drivers are still loaded\n");
    }
    return; //do not continue if bbswitch is used
  }
  if (bbswitch_status() >= 0) {
    return; //do not continue if bbswitch is available at all
  }

  //no bbswitch - attempt vga_switcheroo
  if (switcheroo_status() == 1) {
    //no need to unload driver first, vga_switcheroo should
    //not be available if the driver is not compatible.
    bb_log(LOG_INFO, "Switching dedicated card OFF [vga_switcheroo]\n");
    switcheroo_off();
  }
}//stop_secondary

/// Returns 0 if card is off, 1 if card is on, -1 if not-switchable.
int status_secondary(void){
  int bbstatus = bbswitch_status();
  if (bbstatus >= 0){return bbstatus;}
  bbstatus = switcheroo_status();
  if (bbstatus >= 0){return bbstatus;}
  return -1;
}

/// Checks what methods are available and what drivers are installed.
/// Sets sane defaults for the current environment, also prints
/// debug messages including the found hardware/software.
/// Will print warning message if no switching method is found.
void check_secondary(void){
  //check installed drivers
  bb_config.driver[0] = 0;
  if (module_is_loaded("nvidia")) {
    snprintf(bb_config.driver, BUFFER_SIZE, "nvidia");
    bb_log(LOG_DEBUG, "Detected nvidia driver\n");
  }
  if (module_is_loaded("nouveau")) {
    snprintf(bb_config.driver, BUFFER_SIZE, "nouveau");
    bb_log(LOG_DEBUG, "Detected nouveau driver\n");
  }
  if (bb_config.driver[0] == 0){
    bb_log(LOG_WARNING, "No driver autodetected. Using configured value instead.\n");
  }

  //check switch availability, warn if not availble
  int bbstatus = bbswitch_status();
  if (bbstatus >= 0){
    bb_log(LOG_INFO, "bbswitch detected and will be used.\n");
    return;
  }
  bbstatus = switcheroo_status();
  if (bbstatus >= 0){
    bb_log(LOG_INFO, "vga_switcheroo detected and will be used.\n");
    return;
  }
  bb_log(LOG_WARNING, "No switching method available. The dedicated card will always be on.\n");
}
