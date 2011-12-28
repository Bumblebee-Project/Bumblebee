/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Jaron Viëtor AKA "Thulinma" <jaron@vietors.com>
 * Author: Peter Lekensteyn <lekensteyn@gmail.com>
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "../bblogger.h"
#include "switching.h"

#define BBSWITCH_PATH "/proc/acpi/bbswitch"

/**
 * Reports the status of bbswitch
 *
 * @return SWITCH_OFF if card is off, SWITCH_ON if card is on and SWITCH_UNAVAIL
 * if bbswitch not available
 */
enum switch_state bbswitch_status(void) {
  char buffer[BBS_BUFFER];
  int ret = SWITCH_UNAVAIL;
  FILE * bbs = fopen(BBSWITCH_PATH, "r");
  if (bbs == 0) {
    return SWITCH_UNAVAIL;
  }
  memset(buffer, 0, BBS_BUFFER);
  // skip the PCI Bus ID, a space and 'O'
  if (fseek(bbs, strlen("0000:00:00.0 O"), SEEK_SET) != -1) {
    switch (fgetc(bbs)) {
      case 'F': // value was 0000:00:00.0 OFF
        ret = SWITCH_OFF;
        break;
      case 'N': // value was 0000:00:00.0 ON
        ret = SWITCH_ON;
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

static void bbswitch_write(char *msg) {
  FILE *bbs = fopen(BBSWITCH_PATH, "w");
  if (bbs == 0) {
    bb_log(LOG_ERR, "Could not open %s: %s\n", BBSWITCH_PATH, strerror(errno));
    return;
  }
  fwrite(msg, sizeof msg, strlen(msg) + 1, bbs);
  if (ferror(bbs)) {
    bb_log(LOG_WARNING, "Could not write to %s: %s\n", BBSWITCH_PATH,
            strerror(errno));
  }
  fclose(bbs);
}

/**
 * Turns card on if not already on
 */
void bbswitch_on(void) {
  bbswitch_write("ON\n");
}//bbswitch_on

/**
 * Turns card off if not already off
 */
void bbswitch_off(void) {
  bbswitch_write("OFF\n");
}//bbswitch_off
