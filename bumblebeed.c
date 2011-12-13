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

/*
 * C-coded version of the Bumblebee daemon.
 */

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <grp.h>
#include <signal.h>
#include "bbglobals.h"
#include "bbsocket.h"
#include "bblogger.h"
#include "bbrun.h"

/**
 *  Print a little note on usage 
 */
static void print_usage(int exit_val) {
    // Print help message and exit with exit code
    printf("%s version %s\n\n", bb_config.program_name, TOSTRING(VERSION));
    printf("Usage: %s [options]\n", bb_config.program_name);
    printf("  Options:\n");
    printf("      -d\tRun as daemon.\n");
    printf("      -v\tBe verbose.\n");
    printf("      -h\tShow this help screen.\n");
    printf("\n");
    exit(exit_val);
}

/**
 *  Start the X server by fork-exec 
 */
static int start_x(void) {
    bb_log(LOG_INFO, "Dummy: Starting X server\n");
    return 0;//dummy return value
}

/** 
 * Kill the second X server if any 
 */
static int stop_x(void) {
    bb_log(LOG_INFO, "Stopping X server\n");
    runStop();
    return 1;//always succeeds
}

/** 
 * Turn Dedicated card ON 
 */
static int bb_switch_card_on(void) {
  return 0;//dummy return value
}

/**
 *  Turn Dedicated card OFF 
 */
static int bb_switch_card_off(void) {
  return 0;//dummy return value
}

/**
 *  Read the configuration file 
 */
static int read_configuration() {
    FILE *cf = fopen(CONFIG_FILE, "r");
    if (cf==(NULL)) { /* An error ocurred */
        int err_num = errno;
        assert(cf == NULL);
        switch (err_num) {
            case EACCES:
            case EINVAL:
            case EIO:
            case EISDIR:
            case ELOOP:
            case EMFILE:
            case ENAMETOOLONG:
            case ENFILE:
            case ENOSR:
            case ENOTDIR:
                syslog(LOG_ERR, "Error in config file: %s", strerror(err_num));
        }
    }
    fclose(cf);
    return 0;//placeholder
}

/**
 *  Fork to the background, and exit parent. 
 */
static int daemonize(void) {

    /* Daemon flag, should be reset to zero if fail to daemonize */
    bb_config.is_daemonized = 1;

    //TODO: CHANGE GID ON DAEMON
    /* Change the Group ID of bumblebee */
    struct group *gp;
    errno = 0;
    gp = getgrnam(DEFAULT_BB_GROUP);
    if (gp == NULL) {
        int error_num = errno;
        bb_log(LOG_ERR, "%s\n", strerror(error_num));
        bb_log(LOG_ERR, "There is no \"%s\" group\n", DEFAULT_BB_GROUP);
        exit(EXIT_FAILURE);
    }
    if (setgid((*gp).gr_gid) != -1) {
        bb_log(LOG_ERR, "Could not set the GID of bumblebee");
        exit(EXIT_FAILURE);
    }

    /* Fork off the parent process */
    pid_t bb_pid = fork();
    if (bb_pid < 0) {
        bb_config.is_daemonized = 0;
        bb_log(LOG_ERR, "Could not fork to background\n");
        exit (EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (bb_pid > 0) {
        exit (EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(023);

    /* Create a new SID for the child process */
    pid_t bb_sid = setsid();
    if (bb_sid < 0) {
        /* Log the failure */
        bb_config.is_daemonized = 0;
        bb_log(LOG_ERR, "Could not set SID: %s\n", strerror(errno));
        exit (EXIT_FAILURE);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
        bb_config.is_daemonized = 0;
        bb_log(LOG_ERR, "Could not change to root directory: %s\n", strerror(errno));
        exit (EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    return bb_config.is_daemonized;
}

/**
 *  Handle recieved signals 
 */
static void handle_signal(int sig) {
    switch(sig) {
        case SIGHUP:
            bb_log(LOG_WARNING, "Received %s signal (ignoring...)\n", strsignal(sig));
            break;
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            bb_log(LOG_WARNING, "Received %s signal.\n", strsignal(sig));
            socketClose(&bb_config.bb_socket);//closing the socket terminates the server
            break;
        default:
            bb_log(LOG_WARNING, "Unhandled signal %s\n", strsignal(sig));
            break;
    }
}

/// Socket list structure for use in main_loop.
struct clientsocket{
  int sock;
  int inuse;
  struct clientsocket * next;
};

/// Receive and/or sent data to/from this socket.
/// \param sock Pointer to socket. Assumed to be valid.
void handle_socket(struct clientsocket * C){
  static char buffer[256];
  //since these are local sockets, we can safely assume we get whole messages at a time
  int r = socketRead(&C->sock, buffer, 256);
  if (r > 0){
    switch (buffer[0]){
      case 'S'://status
        //placeholder: return E.
        r = snprintf(buffer, 256, "Ehm... This is a placeholder version. No status available.\n");
        socketWrite(&C->sock, buffer, r);//we assume the write is fully successful.
        break;
      case 'F'://force VirtualGL if possible
      case 'C'://check if VirtualGL is allowed
        //placeholder: return N.
        r = snprintf(buffer, 256, "No, you can't use VirtualGL. This version can't even start X yet, are you kidding me?\n");
        //when answering yes, if (C->inuse == 0){C->inuse = 1; bb_config.appcount++;}
        socketWrite(&C->sock, buffer, r);//we assume the write is fully successful.
        break;
      case 'D'://done, close the socket.
        socketClose(&C->sock);
        break;
      default:
        bb_log(LOG_WARNING, "Unhandled message received: %*s\n", r, buffer);
        break;
    }
  }
}


/* The main loop handles all connections and cleanup.
 * It returns if there are any problems with the listening socket.
 */
static void main_loop(void) {
    int optirun_socket_fd;
    struct clientsocket * first = 0;//pointer to the first socket
    struct clientsocket * last = 0;//pointer to the last socket
    struct clientsocket * curr = 0;//current pointer to a socket
    struct clientsocket * prev = 0;//previous pointer to a socket
    
    bb_log(LOG_INFO, "Started main loop\n");
    /* Listen for Optirun conections and act accordingly */
    while(bb_config.bb_socket != -1) {
        usleep(100000);//sleep 100ms to prevent 100% CPU time usage

        /* Accept a connection. */
        optirun_socket_fd = socketAccept(&bb_config.bb_socket, 1);
        if (optirun_socket_fd >= 0){
          bb_log(LOG_INFO, "Accepted new connection\n", optirun_socket_fd, bb_config.appcount);

          /* add to list of sockets */
          curr = malloc(sizeof(struct clientsocket));
          curr->sock = optirun_socket_fd;
          curr->inuse = 0;
          curr->next = 0;
          if (last == 0){
            first = curr;
            last = curr;
          }else{
            last->next = curr;
            last = curr;
          }
        }

        /* loop through all connections, removing dead ones, receiving/sending data to the rest */
        curr = first;
        prev = 0;
        while (curr != 0){
          if (curr->sock < 0){
            //remove from list
            if (curr->inuse > 0){bb_config.appcount--;}
            if (last == curr){last = prev;}
            if (prev == 0){
              first = curr->next;
              free(curr);
              curr = first;
            }else{
              prev->next = curr->next;
              free(curr);
              curr = prev->next;
            }
          }else{
            //active connection, handle it.
            handle_socket(curr);
            prev = curr;
            curr = curr->next;
          }
        }
    }
}


int main(int argc, char* argv[]) {

    /* Setup signal handling before anything else */
    signal(SIGHUP, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);

    /* TODO: Should check for PID lock, we allow only one instance */

    /* Initializing configuration */
    bb_config.program_name = argv[0];
    bb_config.is_daemonized = 0;

    /* Parse the options, set flags as necessary */
    int c;
    while( (c = getopt(argc, argv, "dvh|help")) != -1) {
        switch(c){
            case 'h':
                print_usage(EXIT_SUCCESS);
                break;
            case 'd':
                bb_config.is_daemonized = 1;
                break;
            case 'v':
                fprintf(stderr, "Warning: Verbose mode not yet implemented\n");
                break;
            default:
                // Unrecognized option
                print_usage(EXIT_FAILURE);
                break;
        }
    }

    /* Init log Mechanism */
    if (bb_init_log()) {
        fprintf(stderr, "Unexpected error, could not initialize log.\n");
        return 1;
    }
    bb_log(LOG_INFO, "%s version %s starting...\n", bb_config.program_name, TOSTRING(VERSION));

    /* Daemonized if daemon flag is activated */
    if (bb_config.is_daemonized) {
        if (daemonize()) {
            bb_log(LOG_ERR, "Error: Bumblebee could not be daemonized\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Initialize communication socket */
    bb_config.bb_socket = socketServer("/tmp/bumblebeed", 1);

    main_loop();

    bb_closelog();
    return (EXIT_SUCCESS);
}
