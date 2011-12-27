/*
 * Copyright (C) 2011 Bumblebee Project
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

#include "../bblogger.h"
#include "switching.h"

/* increase SWITCHERS_COUNT in switching.h when more methods are added */
struct switching_method switching_methods[SWITCHERS_COUNT] = {
  {"bbswitch", 1, bbswitch_status, bbswitch_on, bbswitch_off},
  {"switcheroo", 0, switcheroo_status, switcheroo_on, switcheroo_off}
};

/**
 * Enumerates through available switching methods and try a method
 * 
 * @param name An optional name to find out the status for that name. If NULL,
 * all methods will be probed
 * @return A switching method if available, NULL otherwise
 */
struct switching_method *switcher_detect(char *name) {
  int i;
  switcher = NULL;
  for (i = 0; i<SWITCHERS_COUNT; ++i) {
    /* If the status is 0 or 1, the method is usable */
    if ((name && name == switching_methods[i].name) ||
            (switching_methods[i].status() != SWITCH_UNAVAIL)) {
      switcher = &switching_methods[i];
      break;
    }
  }
  return switcher;
}

enum switch_state switch_status(void) {
  if (switcher) {
    return switcher->status();
  }
  return SWITCH_UNAVAIL;
}

enum switch_state switch_on(void) {
  if (switcher) {
    if (switcher->status() == SWITCH_ON) {
      return SWITCH_ON;
    }
    bb_log(LOG_INFO, "Switching dedicated card ON [%s]\n", switcher->name);
    switcher->on();
    return switcher->status();
  }
  return SWITCH_UNAVAIL;
}

enum switch_state switch_off(void) {
  if (switcher) {
    if (switcher->status() == SWITCH_OFF) {
      return SWITCH_OFF;
    }
    bb_log(LOG_INFO, "Switching dedicated card OFF [%s]\n", switcher->name);
    switcher->off();
    return switcher->status();
  }
  return SWITCH_UNAVAIL;
}