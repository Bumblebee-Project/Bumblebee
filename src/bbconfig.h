/*
 * Copyright (c) 2011, The Bumblebee Project
 * Author: Joaquín Ignacio Aramendía samsagax@gmail.com
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

/*
 * bbconfig.h: Bumblebee configuration file handler
 */

#include <unistd.h> //for pid_t

/* Daemon states */
#define BB_DAEMON 1
#define BB_NODEAMON 0

/* Verbosity levels */
enum verbosity_level {
    VERB_NONE,
    VERB_ERR,
    VERB_WARN,
    VERB_INFO,
    VERB_DEBUG,
    VERB_ALL
};

/* Running modes */
enum bb_run_mode {
    BB_RUN_SERVER = 0,
    BB_RUN_DAEMON = 1,
    BB_RUN_APP = 2,
    BB_RUN_STATUS = 4,
    BB_RUN_EXIT = 99
};

/* String buffer size */
#define BUFFER_SIZE 256

/* Structure containing the status of the application */
struct bb_status_struct {
    char program_name[BUFFER_SIZE]; /// How this application was called.
    enum verbosity_level verbosity; ///Verbosity level of messages.
    int bb_socket; /// The socket file descriptor of the application.
    unsigned int appcount; /// Count applications using the X server.
    char errors[BUFFER_SIZE]; /// Error message if any. First byte is 0 otherwise.
    enum bb_run_mode runmode; /// Running mode.
    pid_t x_pid;
};

/* Structure containing the configuration. */
struct bb_config_struct {
    char x_display[BUFFER_SIZE]; /// X display number to use.
    char x_conf_file[BUFFER_SIZE]; /// Path to the X configuration file.
    char bb_conf_file[BUFFER_SIZE]; /// Path to the bumblebeed configuration file.
    char ld_path[BUFFER_SIZE]; /// LD_LIBRARY_PATH to launch applications.
    char socket_path[BUFFER_SIZE]; /// Path to the server socket.
    char gid_name[BUFFER_SIZE]; /// Group name for setgid.
    int pm_enabled; /// Whether power management is enabled.
    int stop_on_exit; /// Whether to stop the X server on last optirun instance exit.
    int fallback_start; /// Wheter the application should be launched on the integrated card when X is not available.
    char vgl_compress[BUFFER_SIZE]; /// VGL transport method.
    char driver[BUFFER_SIZE]; /// Driver to use (nvidia or nouveau).
    int pci_bus_id; /// PCI Bus ID of the discrete video card
    int card_shutdown_state;
};

extern struct bb_status_struct bb_status;
extern struct bb_config_struct bb_config;

/// Read commandline parameters and config file.
void init_config(int argc, char ** argv);
void config_dump(void);