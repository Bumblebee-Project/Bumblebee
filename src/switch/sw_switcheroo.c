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
 * Reports the status of vga switcheroo
 *
 * @return 0 if card is off, 1 if card is on and -1 if switcheroo not available
 */
int switcheroo_status(void) {
  char buffer[BBS_BUFFER];
  int ret = -1;
  FILE * bbs = fopen("/sys/kernel/debug/vgaswitcheroo/switch", "r");
  if (bbs == 0) {
    return -1;
  }
  while (fgets(buffer, BBS_BUFFER, bbs)) {
    if (strlen(buffer) > strlen("0:DIS: :Pwr") &&
            !strncmp(buffer + 2, "DIS", 3)) {//found the DIS line
      // compare the first char after "0:DIS: :"
      switch (buffer[strlen("0:DIS: :")]) {
        case 'P': // Pwr
          ret = 1;
          break;
        case 'O':
          ret = 0;
          break;
      }
    }
  }
  fclose(bbs);
  return ret;
}//switcheroo_status

/**
 * Turns card on if not already on
 */
void switcheroo_on(void) {
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

/**
 * Turns card off if not already off.
 */
void switcheroo_off(void) {
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
