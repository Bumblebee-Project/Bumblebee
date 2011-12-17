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

#include "bbglobals.h"

/* TODO: TRANSFER MACROS TO CONFIGURATION STRUCT*/
#define DAEMON_NAME "bumblebee"
#define DEFAULT_BB_GROUP "bumblebee"
#define CONFIG_FILE "/etc/bumblebee/bumblebee.conf"

/* Structure containing the status of the application */
struct bb_status_struct {
    char* program_name; /// How this application was called.
    int verbosity; ///Verbosity level of messages.
    int bb_socket; /// The socket file descriptor of the application.
    unsigned int appcount; /// Count applications using the X server.
    char errors[BUFFER_SIZE]; /// Error message if any. First byte is 0 otherwise.
    int runmode; /// Running mode.
}

/* Structure containing the server configuration. Only valid if running 
 * as daemon/server */
struct bb_server_config_struct {
    char x_display[BUFFER_SIZE]; /// X display number to use.
    char x_conf_file[BUFFER_SIZE]; /// Path to the X configuration file to use.
    char ld_path[BUFFER_SIZE]; /// LD_LIBRARY_PATH to launch applications.
    char socket_path[BUFFER_SIZE]; /// Path to the server socket.
    int pm_enabled; /// Wether power management is enabled.
    int stop_on_exit; /// Wether to stop the X server on last optirun instance exit.
}

/* Structure containing the client configuration. Only valid when running 
 * as cleint */
struct bb_client_config_struct {
    char* vgl_compress;
}

extern struct bb_status_struct bb_status;
extern struct bb_server_config_struct bb_s_config;
extern struct bb_client_config_struct bb_c_config;

