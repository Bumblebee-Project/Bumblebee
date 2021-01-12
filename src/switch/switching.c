/*
 * Copyright (c) 2011-2013, The Bumblebee Project
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

#include <stddef.h>
#include <string.h>
#include "../bblogger.h"
#include "switching.h"

/* increase SWITCHERS_COUNT in switching.h when more methods are added */
struct switching_method switching_methods[SWITCHERS_COUNT] = {
  {"bbswitch", 1, bbswitch_status, bbswitch_is_available,
          bbswitch_on, bbswitch_off},
  {"switcheroo", 0, switcheroo_status, switcheroo_is_available,
          switcheroo_on, switcheroo_off}
};

struct switching_method *switcher = NULL;

/**
 * Enumerates through available switching methods and try a method
 * 
 * @param name An optional name to find out the status for that name. If NULL,
 * all methods will be probed
 * @param info A struct containing information which would help with the
 * decision whether bbswitch is usable or not
 * @return A switching method if available, NULL otherwise
 */
struct switching_method *switcher_detect(const char *name,
        struct switch_info info) {
  int i;
  switcher = NULL;
  for (i = 0; i<SWITCHERS_COUNT; ++i) {
    /* If the status is 0 or 1, the method is usable */
    if ((!name || strcmp(name, switching_methods[i].name) == 0) &&
            switching_methods[i].is_available(info)) {
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
