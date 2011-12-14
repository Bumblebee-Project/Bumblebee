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
#define CONFIG_FILE "/etc/bumblebee/bumblebee.conf"

/* Daemon states */
#define BB_DAEMON 1
#define BB_NODEAMON 0

/* Verbosity levels */
#define VERB_NONE 0
#define VERB_ERR 1
#define VERB_WARN 2
#define VERB_INFO 3
#define VERB_DEBUG 4
#define VERB_ALL 4

/* Running modes */
#define BB_RUN_DAEMON 0
#define BB_RUN_APP 1
#define BB_RUN_STATUS 2

/* String buffer size */
#define BUFFER_SIZE 256

/* For conversting defines to strings */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/* Structure containing the daemon configuration and status */
struct bb_config_struct {
    char* program_name; /// How this application was called.
    int verbosity; /// Verbosity level of messages.
    int is_daemonized; /// 1 if running as daemon, 0 otherwise.
    int bb_socket; /// The main socket of the application.
    unsigned int appcount; /// Count of applications using the secondary X.
    char errors[BUFFER_SIZE]; /// Error message, if any. First byte is 0 otherwise.
    int runmode; /// See running modes above.
    int xdisplay; /// Number of the used X display for VirtualGL.
    char xconf[BUFFER_SIZE]; /// Filename for secondary X xorg.conf file.
    char ldpath[BUFFER_SIZE]; /// Path for LD for vglrun'ed applications.
    char socketpath[BUFFER_SIZE]; /// Filename for bumblebee communication socket.
};

extern struct bb_config_struct bb_config;
