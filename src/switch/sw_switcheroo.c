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
#include <unistd.h>
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
 * Whether vga_switcheroo is available for use
 *
 * @param info A struct containing information which would help with the
 * decision whether vga_switcheroo is usable or not
 * @return 1 if available for use for PM, 0 otherwise
 */
int switcheroo_is_available(struct switch_info info) {
  if (strcmp(info.configured_pm, "switcheroo") != 0) {
    bb_log(LOG_INFO, "Skipping switcheroo PM method because it is not"
            " explicitly selected in the configuration.\n");
    return 0;
  }
  if (strcmp("nouveau", info.driver)) {
    /* switcheroo cannot be used with drivers other than nouveau */
    bb_log(LOG_WARNING, "vga_switcheroo can only be used with the nouveau"
            " driver, skipping method.\n");
    return 0;
  }
  return 1;
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
