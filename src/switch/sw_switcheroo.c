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

#define SWITCHEROO_PATH "/sys/kernel/debug/vgaswitcheroo/switch"

/**
 * Reports the status of vga switcheroo
 *
 * @return SWITCH_OFF if card is off, SWITCH_ON if card is on and SWITCH_UNAVAIL
 * if switcheroo not available
 */
int switcheroo_status(void) {
  char buffer[BBS_BUFFER];
  int ret = SWITCH_UNAVAIL;
  FILE * bbs = fopen(SWITCHEROO_PATH, "r");
  if (bbs == 0) {
    return SWITCH_UNAVAIL;
  }
  while (fgets(buffer, BBS_BUFFER, bbs)) {
    if (strlen(buffer) > strlen("0:DIS: :Pwr") &&
            !strncmp(buffer + 2, "DIS", 3)) {//found the DIS line
      // compare the first char after "0:DIS: :"
      switch (buffer[strlen("0:DIS: :")]) {
        case 'P': // Pwr
          ret = SWITCH_ON;
          break;
        case 'O':
          ret = SWITCH_OFF;
          break;
      }
    }
  }
  fclose(bbs);
  return ret;
}//switcheroo_status

static void switcheroo_write(char *msg) {
  FILE * bbs = fopen(SWITCHEROO_PATH, "w");
  if (bbs == 0) {
    bb_log(LOG_ERR, "Could not open %s: %s\n", SWITCHEROO_PATH,
            strerror(errno));
    return;
  }
  fwrite(msg, sizeof msg, strlen(msg) + 1, bbs);
  if (ferror(bbs)) {
    bb_log(LOG_WARNING, "Could not write to %s: %s\n", SWITCHEROO_PATH,
            strerror(errno));
  }
  fclose(bbs);
}

/**
 * Turns card on if not already on
 */
void switcheroo_on(void) {
  switcheroo_write("ON\n");
}//switcheroo_on

/**
 * Turns card off if not already off.
 */
void switcheroo_off(void) {
  switcheroo_write("OFF\n");
}//switcheroo_off
