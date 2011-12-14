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
    printf("Usage: %s [options] -- [application to run] [application options]\n", bb_config.program_name);
    printf("  Options:\n");
    printf("      -d\tRun as daemon.\n");
    printf("      -c\tBe quit.\n");
    printf("      -v\tBe verbose.\n");
    printf("      -V\tBe VERY verbose.\n");
    printf("      -r\tRun application, do not start listening.\n");
    printf("      -s\tPrint current status, do not start listening.\n");
    printf("      -x [PATH]\txorg.conf file to use.\n");
    printf("      -X #\tX display number to use.\n");
    printf("      -l [PATH]\tLD driver path to use.\n");
    printf("      -u [PATH]\tUnix socket to use.\n");
    printf("      -h\tShow this help screen.\n");
    printf("\n");
    printf("When called as optirun, -r is assumed unless -d is set.\n");
    printf("If -r is set but no application is given, -s is assumed.\n");
    printf("\n");
    exit(exit_val);
}

/**
 *  Start the X server by fork-exec 
 */
void start_x(void) {
  bb_log(LOG_INFO, "Starting X server\n");
  char buffer[BUFFER_SIZE];
  snprintf(buffer, BUFFER_SIZE, "X -config %s -sharevts -nolisten tcp -noreset :%i", bb_config.xconf, bb_config.xdisplay);
  runFork(buffer);
}

/** 
 * Kill the second X server if any 
 */
void stop_x(void) {
  if (isRunning()){
    bb_log(LOG_INFO, "Stopping X server\n");
    runStop();
  }
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
 *  Handle recieved signals - except SIGCHLD, which is handled in bbrun.c
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
            runStop();
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
  static char buffer[BUFFER_SIZE];
  //since these are local sockets, we can safely assume we get whole messages at a time
  int r = socketRead(&C->sock, buffer, BUFFER_SIZE);
  if (r > 0){
    switch (buffer[0]){
      case 'S'://status
        if (bb_config.errors[0] != 0){
          r = snprintf(buffer, BUFFER_SIZE, "Error (%s): %s\n", TOSTRING(VERSION), bb_config.errors);
        }else{
          if (isRunning()){
            r = snprintf(buffer, BUFFER_SIZE, "Ready (%s). X is PID %i, %i applications using bumblebeed.\n", TOSTRING(VERSION), curr_id, bb_config.appcount);
          }else{
            r = snprintf(buffer, BUFFER_SIZE, "Ready (%s). X inactive.\n", TOSTRING(VERSION));
          }
        }
        socketWrite(&C->sock, buffer, r);//we assume the write is fully successful.
        break;
      case 'F'://force VirtualGL if possible
      case 'C'://check if VirtualGL is allowed
        /// \todo Handle power management cases and powering card on/off.
        //no X? attempt to start it
        if (!isRunning()){
          start_x();
          usleep(100000);//sleep 100ms to give X a chance to fail
          if (!isRunning()){
            snprintf(bb_config.errors, BUFFER_SIZE, "X failed to start!");
            bb_log(LOG_ERR, "X failed to start!\n");
          }
        }
        if (isRunning()){
          r = snprintf(buffer, BUFFER_SIZE, "Yes. X is active.\n");
          if (C->inuse == 0){
            C->inuse = 1;
            bb_config.appcount++;
          }
        }else{
          if (bb_config.errors[0] != 0){
            r = snprintf(buffer, BUFFER_SIZE, "No - error: %s\n", bb_config.errors);
          }else{
            r = snprintf(buffer, BUFFER_SIZE, "No, secondary X is not active.\n");
          }
        }
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

        //stop X if there is no need to keep it running
        if (isRunning() && (bb_config.appcount == 0)){
          stop_x();
        }

        /* Accept a connection. */
        optirun_socket_fd = socketAccept(&bb_config.bb_socket, SOCK_NOBLOCK);
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

    /* Initializing configuration */
    bb_config.program_name = argv[0];
    bb_config.is_daemonized = 0;
    bb_config.verbosity = VERB_WARN;
    bb_config.errors[0] = 0;//no errors, yet :-)
    bb_config.xdisplay = 8;
    snprintf(bb_config.xconf, BUFFER_SIZE, "/etc/bumblebee/xorg.conf.nvidia");
    snprintf(bb_config.ldpath, BUFFER_SIZE, "/usr/lib64/nvidia-current");
    snprintf(bb_config.socketpath, BUFFER_SIZE, "/var/run/bumblebee.socket");
    bb_config.runmode = BB_RUN_DAEMON;
    if ((strcmp(bb_config.program_name, "optirun") == 0) || (strcmp(bb_config.program_name, "./optirun") == 0)){
      bb_config.runmode = BB_RUN_APP;
    }
    
    /* Parse the options, set flags as necessary */
    int c;
    while( (c = getopt(argc, argv, "+dcvVx:X:l:u:h|help")) != -1) {
        switch(c){
            case 'h'://help
                print_usage(EXIT_SUCCESS);
                break;
            case 'd'://daemonize
                bb_config.is_daemonized = 1;
                bb_config.runmode = BB_RUN_DAEMON;
                break;
            case 'c'://clean run (no output)
                bb_config.verbosity = VERB_NONE;
                break;
            case 'v'://verbose
                bb_config.verbosity = VERB_INFO;
                break;
            case 'V'://VERY verbose (debug mode)
                bb_config.verbosity = VERB_DEBUG;
                break;
            case 'r'://run application
                bb_config.runmode = BB_RUN_APP;
                break;
            case 's'://show status
                bb_config.runmode = BB_RUN_STATUS;
                break;
            case 'x'://xorg.conf path
                snprintf(bb_config.xconf, BUFFER_SIZE, "%s", optarg);
                break;
            case 'X'://X display number
                bb_config.xdisplay = atoi(optarg);
                break;
            case 'l'://LD driver path
                snprintf(bb_config.ldpath, BUFFER_SIZE, "%s", optarg);
                break;
            case 'u'://Unix socket to use
                snprintf(bb_config.socketpath, BUFFER_SIZE, "%s", optarg);
                break;
            default:
                // Unrecognized option
                print_usage(EXIT_FAILURE);
                break;
        }
    }

    /* change runmode to status if no application given to run
     * and current runmode is run application.
     */
    if ((bb_config.runmode == BB_RUN_APP) && (optind >= argc)){
      bb_config.runmode = BB_RUN_STATUS;
    }

    /* Init log Mechanism */
    if (bb_init_log()) {
        fprintf(stderr, "Unexpected error, could not initialize log.\n");
        return 1;
    }
    bb_log(LOG_DEBUG, "%s version %s starting...\n", bb_config.program_name, TOSTRING(VERSION));

    /* Daemonized if daemon flag is activated */
    if (bb_config.is_daemonized) {
        if (daemonize()) {
            bb_log(LOG_ERR, "Error: Bumblebee could not be daemonized\n");
            exit(EXIT_FAILURE);
        }
    }

    if (bb_config.runmode == BB_RUN_DAEMON){
      /* Initialize communication socket, enter main loop */
      bb_config.bb_socket = socketServer(bb_config.socketpath, SOCK_NOBLOCK);
      main_loop();
    }else{
      /* Connect to listening daemon */
      bb_config.bb_socket = socketConnect(bb_config.socketpath, SOCK_NOBLOCK);
      if (bb_config.bb_socket < 0){
        bb_log(LOG_ERR, "Could not connect to bumblebee daemon - is it running?\n");
        bb_closelog();
        return EXIT_FAILURE;
      }
      char buffer[BUFFER_SIZE];
      int r;

      /* Request status */
      if (bb_config.runmode == BB_RUN_STATUS){
        r = snprintf(buffer, BUFFER_SIZE, "Status?");
        socketWrite(&bb_config.bb_socket, buffer, r);
        while (bb_config.bb_socket != -1){
          r = socketRead(&bb_config.bb_socket, buffer, BUFFER_SIZE);
          if (r > 0){
            printf("Bumblebee status: %*s\n", r, buffer);
            socketClose(&bb_config.bb_socket);
          }
        }
      }

      /* Run given application */
      if (bb_config.runmode == BB_RUN_APP){
        r = snprintf(buffer, BUFFER_SIZE, "Checking availability...");
        socketWrite(&bb_config.bb_socket, buffer, r);
        while (bb_config.bb_socket != -1){
          r = socketRead(&bb_config.bb_socket, buffer, BUFFER_SIZE);
          if (r > 0){
            bb_log(LOG_INFO, "Response: %*s\n", r, buffer);
            switch (buffer[0]){
              case 'N': //No, run normally.
                socketClose(&bb_config.bb_socket);
                bb_log(LOG_WARNING, "Running application normally.\n");
                runApp(argc - optind, argv + optind);
                break;
              case 'Y': //Yes, run through vglrun
                bb_log(LOG_INFO, "Running application through vglrun.\n");
                snprintf(buffer, BUFFER_SIZE, "vglrun -c proxy -d :%i -ld %s --", bb_config.xdisplay, bb_config.ldpath);
                runApp2(buffer, argc - optind, argv + optind);
                socketClose(&bb_config.bb_socket);
                break;
              default: //Something went wrong - output and exit.
                bb_log(LOG_ERR, "Problem: %*s\n", r, buffer);
                socketClose(&bb_config.bb_socket);
                break;
            }
          }
        }
      }
    }

    bb_closelog();
    return (EXIT_SUCCESS);
}
