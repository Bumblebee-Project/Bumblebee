/*
 * Copyright (C) 2011 Bumblebee Project
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

/*
 * bbswitch-related functions for Bumblebee
 */
#include "bbswitch.h"
#include "bblogger.h"
#include <stdio.h>
#include <string.h>

#define BBS_BUFFER 100

/// Returns 0 if card is off, 1 if card is on, -1 if bbswitch not active.
/// In other words: 0 means off, anything else means on.
int bbswitch_status(){
  char buffer[BBS_BUFFER];
  int i;
  FILE * bbs = fopen("/proc/acpi/bbswitch", "r");
  if (bbs == 0){return -1;}
  for (i = 0; i < BBS_BUFFER; ++i){buffer[i] = 0;}
  fread(buffer, BBS_BUFFER-1, 1, bbs);
  fclose(bbs);
  //we do not check the return value of fread, since it
  //apparently always returns 0 for some reason :S
  for (i = 0; i < BBS_BUFFER; ++i){
    if (buffer[i] == ' '){//find the space
      if (buffer[i+2] == 'F'){return 0;}//OFF
      if (buffer[i+2] == 'N'){return 1;}//ON
    }
    if (buffer[i] == 0){return -1;}//end of string, stop search
  }
  return -1;//space not found - assume bbswitch isn't working
}//bbswitch_status

/// Turns card on if not already on.
void bbswitch_on(){
  int r;
  r = bbswitch_status();
  if (r != 0){
    if (r == 1){
      bb_log(LOG_INFO, "Card already on, not turning dedicated card on.");
    }else{
      bb_log(LOG_WARNING, "bbswitch unavailable, not turning dedicated card on.");
    }
    return;
  }
  FILE * bbs = fopen("/proc/acpi/bbswitch", "w");
  if (bbs == 0){
    bb_log(LOG_ERR, "Could not access bbswitch module.\n");
    return;
  }
  r = fwrite("ON", 2, 1, bbs);
  fclose(bbs);
  if (r < 2){bb_log(LOG_WARNING, "bbswitch isn't listening to us!");}
  r = bbswitch_status();
  if (r != 1){
    bb_log(LOG_ERR, "Failed to turn dedicated card on!");
  }
}//bbswitch_on

/// Turns card off if not already off.
void bbswitch_off(){
  int r;
  r = bbswitch_status();
  if (r != 1){
    if (r == 0){
      bb_log(LOG_INFO, "Card already off, not turning dedicated card off.");
    }else{
      bb_log(LOG_WARNING, "bbswitch unavailable, not turning dedicated card off.");
    }
    return;
  }
  FILE * bbs = fopen("/proc/acpi/bbswitch", "w");
  if (bbs == 0){
    bb_log(LOG_ERR, "Could not access bbswitch module.\n");
    return;
  }
  r = fwrite("OFF", 3, 1, bbs);
  fclose(bbs);
  if (r < 3){bb_log(LOG_WARNING, "bbswitch isn't listening to us!");}
  r = bbswitch_status();
  if (r != 0){
    bb_log(LOG_ERR, "Failed to turn dedicated card off!");
  }
}//bbswitch_off
