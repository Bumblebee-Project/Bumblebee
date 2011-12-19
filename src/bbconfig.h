/*
 * Copyright (c) 2011, The Bumblebee Project
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
 * bbconfig.h: Bumblebee configuration file handler
 */

#include <unistd.h> //for pid_t

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

/* Structure containing the status of the application */
struct bb_status_struct {
    char* program_name; /// How this application was called.
    int verbosity; ///Verbosity level of messages.
    int bb_socket; /// The socket file descriptor of the application.
    unsigned int appcount; /// Count applications using the X server.
    char errors[BUFFER_SIZE]; /// Error message if any. First byte is 0 otherwise.
    int is_daemonized; /// Whether the application is daemonized or not.
    int runmode; /// Running mode.
    pid_t x_pid;
};

/* Structure containing the configuration. */
struct bb_config_struct {
    char x_display[BUFFER_SIZE]; /// X display number to use.
    char x_conf_file[BUFFER_SIZE]; /// Path to the X configuration file to use.
    char ld_path[BUFFER_SIZE]; /// LD_LIBRARY_PATH to launch applications.
    char socket_path[BUFFER_SIZE]; /// Path to the server socket.
    char gid_name[BUFFER_SIZE]; /// Group name for setgid.
    int pm_enabled; /// Whether power management is enabled.
    int stop_on_exit; /// Whether to stop the X server on last optirun instance exit.
    char* vgl_compress; /// VGL transport method
};

extern struct bb_status_struct bb_status;
extern struct bb_config_struct bb_config;

/// Read the configuration file
int read_configuration( void );
