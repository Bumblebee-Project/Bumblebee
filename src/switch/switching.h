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


#pragma once

/* Buffer size for result from reading files for switching methods */
#define BBS_BUFFER 100

enum switch_state {
  SWITCH_ON = 1,
  SWITCH_OFF = 0,
  SWITCH_UNAVAIL = -1
};

/* information that could be useful for use in is_available */
struct switch_info {
  char *driver; /* possible values are nouveau and nvidia */
  const char *configured_pm; /* configured PM method, NOT the detected one */
};

struct switching_method {
  char *name; /* a short name for informational / logging purposes */
  int need_driver_unloaded; /* 1 if the method needs drivers to be unloaded
                              * when disabling the card, 0 otherwise*/
  enum switch_state (*status)(void); /* reports status: off (0), on (1),
                                      * unavailable (-1) */
  int (*is_available)(struct switch_info); /* returns 1 if available for use,
                                            * 0 otherwise */
  void (*on)(void); /* attempts to enable a card */
  void (*off)(void); /* attempts to disable a card */
};

enum switch_state nouveau_status(void);
int nouveau_is_available(struct switch_info);
void nouveau_on(void);
void nouveau_off(void);

enum switch_state bbswitch_status(void);
int bbswitch_is_available(struct switch_info);
void bbswitch_on(void);
void bbswitch_off(void);

enum switch_state switcheroo_status(void);
int switcheroo_is_available(struct switch_info);
void switcheroo_on(void);
void switcheroo_off(void);

/* number of switchers as defined in switching.c */
#define SWITCHERS_COUNT 3
struct switching_method switching_methods[SWITCHERS_COUNT];

/* A switching method that can be used or NULL if none */
struct switching_method *switcher;

struct switching_method *switcher_detect(const char *name, struct switch_info);
enum switch_state switch_status(void);
enum switch_state switch_on(void);
enum switch_state switch_off(void);
