/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Joaquín Ignacio Aramendía <samsagax@gmail.com>
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
 * C-coded version of the Optirun client.
 */

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "bbcommon.h"


//TODO: Error and signal handling.

/* Structure containing the daemon configuration and status */
struct {
    /* The name which the program was called */
    char* program_name;

    struct {
        int sock_fd;
        struct sockaddr_un sock_name;
    } optirun_socket;
} optirun_config;

/* Default buffer size */
static const size_t DEFAULT_BUFFER_SIZE = 256;

static int init_socket(void) {
    int sock_fd;
    struct sockaddr_un name;
    /* Create the socket */
    optirun_config.optirun_socket.sock_fd = socket (PF_LOCAL, SOCK_STREAM, 0);
    /* Store the server’s name in the socket address */
    optirun_config.optirun_socket.sock_name.sun_family = AF_LOCAL;
    strcpy(optirun_config.optirun_socket.sock_name.sun_path, BBS_PATH);
    return 0;
}

static int close_socket(void) {

}

static int request_server(void) {

}

static int run_program(void) {

}


int main(int argc, char* argv[]) {
    optirun_config.program_name = argv[0];

    /*
        Set signal handling
    */


    /*
        Parse options and set program and arguments
    */

    init_socket();
    if ( request_server() == 0) {
        run_program();
    }
    close_socket();

    return 0;
}
