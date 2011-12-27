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

#include "../bblogger.h"
#include "switching.h"

/**
 * Reports the status of bbswitch
 *
 * @return 0 if card is off, 1 if card is on and -1 if bbswitch not available
 */
int bbswitch_status(void) {
  char buffer[BBS_BUFFER];
  int ret = -1;
  FILE * bbs = fopen("/proc/acpi/bbswitch", "r");
  if (bbs == 0) {
    return -1;
  }
  memset(buffer, 0, BBS_BUFFER);
  // skip the PCI Bus ID, a space and 'O'
  if (fseek(bbs, strlen("0000:00:00.0 O"), SEEK_SET) != -1) {
    switch (fgetc(bbs)) {
      case 'F': // value was 0000:00:00.0 OFF
        ret = 0;
        break;
      case 'N': // value was 0000:00:00.0 ON
        ret = 1;
        break;
      default:
        // this should never happen unless the behavior of the bbswitch kernel
        // module has changed. If no device was registered, the procfs entry
        // should not exist either
        break;
    }
  }
  fclose(bbs);
  return ret;
}//bbswitch_status

/**
 * Turns card on if not already on
 */
void bbswitch_on(void) {
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

/**
 * Turns card off if not already off
 */
void bbswitch_off(void) {
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
