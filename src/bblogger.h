/*
 * Copyright (c) 2011-2013, The Bumblebee Project
 * Author: Joaquín Ignacio Aramendía samsagax@gmail.com
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
 * bblogger.c: loggin functions for bumblebee daemon and client
 */
/* necessary for all LOG_ constants */
#include <syslog.h>

#ifndef _BBLOGGER
#define _BBLOGGER

/**
 * Initialize log capabilities. Return 0 on success 
 */
int bb_init_log(void);

/**
 * Log a message to the current log mechanism.
 * Try to keep log messages less than 80 characters.
 */
void bb_log(int priority, char* msg_format, ...);

/** 
 * Close logging mechanism 
 */
void bb_closelog(void);

/** Will check the xorg output pipe and parse any waiting messages.
 * Doesn't take any parameters and doesn't return anything.
 */
void check_xorg_pipe(void);

#endif
