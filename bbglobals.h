/// \file bbglobals.h Contains definitions for global variables used throughout the application.

/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Joaquín Ignacio Aramendía <samsagax@gmail.com>
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
#pragma once

/* TODO: TRANSFER MACROS TO CONFIGURATION STRUCT*/
#define DAEMON_NAME "bumblebee"
#define DEFAULT_BB_GROUP "bumblebee"
#define BBS_PATH "/var/run/bumblebee.socket"
#define PID_FILE "/var/run/bumblebeed.pid"
#define CONFIG_FILE "/etc/bumblebee/bumblebee.conf"
#define X_DISPLAY_NUM "8"

/* Optirun signals */
#define Q_WANT_X  1
#define Q_DONE_X  3

/* Daemon responses */
#define X_ALIVE 4
#define X_DEAD  5

/* Daemon states */
#define BB_DAEMON 1
#define BB_NODEAMON 0

/* Default buffer size */
#define DEFAULT_BUFFER_SIZE 256

/* Structure containing the daemon configuration and status */
struct bb_config_struct {
    /* The name which the program was called */
    char* program_name;
    int is_daemonized;

    /* Communication socket */
    int bb_socket;
};

extern struct bb_config_struct bb_config;
