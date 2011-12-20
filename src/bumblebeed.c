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
 * C-coded version of the Bumblebee daemon and optirun.
 */

#include "config.h"
#include "bbconfig.h"
#include "bbsocket.h"
#include "bblogger.h"
#include "bbsecondary.h"
#include "bbrun.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <grp.h>
#include <signal.h>
#include <time.h>

/**
 * Change GID and umask of the daemon
 */
static int bb_chgid(void) {
  /* Change the Group ID of bumblebee */
  struct group *gp;
  errno = 0;
  gp = getgrnam(bb_config.gid_name);
  if (gp == NULL) {
    int error_num = errno;
    bb_log(LOG_ERR, "%s\n", strerror(error_num));
    bb_log(LOG_ERR, "There is no \"%s\" group\n", bb_config.gid_name);
    exit(EXIT_FAILURE);
  }
  if (setgid(gp->gr_gid) != 0) {
    bb_log(LOG_ERR, "Could not set the GID of bumblebee: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  /* Change the file mode mask */
  umask(023);
  return 0;
}

/**
 *  Fork to the background, and exit parent.
 */
static void daemonize(void) {
  /* Fork off the parent process */
  pid_t bb_pid = fork();
  if (bb_pid < 0) {
    bb_log(LOG_ERR, "Could not fork to background\n");
    exit(EXIT_FAILURE);
  }

  /* If we got a good PID, then we can exit the parent process. */
  if (bb_pid > 0) {
    exit(EXIT_SUCCESS);
  }

  /* Create a new SID for the child process */
  pid_t bb_sid = setsid();
  if (bb_sid < 0) {
    bb_log(LOG_ERR, "Could not set SID: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Change the current working directory */
  if ((chdir("/")) < 0) {
    bb_log(LOG_ERR, "Could not change to root directory: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Close out the standard file descriptors */
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

/**
 *  Handle recieved signals - except SIGCHLD, which is handled in bbrun.c
 */
static void handle_signal(int sig) {
  switch (sig) {
    case SIGHUP:
      bb_log(LOG_WARNING, "Received %s signal (ignoring...)\n", strsignal(sig));
      break;
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
      bb_log(LOG_WARNING, "Received %s signal.\n", strsignal(sig));
      socketClose(&bb_status.bb_socket); //closing the socket terminates the server
      break;
    default:
      bb_log(LOG_WARNING, "Unhandled signal %s\n", strsignal(sig));
      break;
  }
}

/// Socket list structure for use in main_loop.

struct clientsocket {
  int sock;
  int inuse;
  struct clientsocket * next;
};

/// Receive and/or sent data to/from this socket.
/// \param sock Pointer to socket. Assumed to be valid.

static void handle_socket(struct clientsocket * C) {
  static char buffer[BUFFER_SIZE];
  //since these are local sockets, we can safely assume we get whole messages at a time
  int r = socketRead(&C->sock, buffer, BUFFER_SIZE);
  if (r > 0) {
    switch (buffer[0]) {
      case 'S'://status
        if (bb_status.errors[0] != 0) {
          r = snprintf(buffer, BUFFER_SIZE, "Error (%s): %s\n", GITVERSION, bb_status.errors);
        } else {
          if (bb_is_running(bb_status.x_pid)) {
            r = snprintf(buffer, BUFFER_SIZE, "Ready (%s). X is PID %i, %i applications using bumblebeed.\n", GITVERSION, bb_status.x_pid, bb_status.appcount);
          } else {
            r = snprintf(buffer, BUFFER_SIZE, "Ready (%s). X inactive.\n", GITVERSION);
          }
        }
        socketWrite(&C->sock, buffer, r); //we assume the write is fully successful.
        break;
      case 'F'://force VirtualGL if possible
      case 'C'://check if VirtualGL is allowed
        /// \todo Handle power management cases and powering card on/off.
        //no X? attempt to start it
        if (!bb_is_running(bb_status.x_pid)) {
          start_secondary();
        }
        if (bb_is_running(bb_status.x_pid)) {
          r = snprintf(buffer, BUFFER_SIZE, "Yes. X is active.\n");
          if (C->inuse == 0) {
            C->inuse = 1;
            bb_status.appcount++;
          }
        } else {
          if (bb_status.errors[0] != 0) {
            r = snprintf(buffer, BUFFER_SIZE, "No - error: %s\n", bb_status.errors);
          } else {
            r = snprintf(buffer, BUFFER_SIZE, "No, secondary X is not active.\n");
          }
        }
        socketWrite(&C->sock, buffer, r); //we assume the write is fully successful.
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
  struct clientsocket * first = 0; //pointer to the first socket
  struct clientsocket * last = 0; //pointer to the last socket
  struct clientsocket * curr = 0; //current pointer to a socket
  struct clientsocket * prev = 0; //previous pointer to a socket
  time_t lastcheck = 0;

  bb_log(LOG_INFO, "Started main loop\n");
  /* Listen for Optirun conections and act accordingly */
  while (bb_status.bb_socket != -1) {
    usleep(100000); //sleep 100ms to prevent 100% CPU time usage

    //every five seconds
    if (time(0) - lastcheck > 5) {
      lastcheck = time(0);
      //stop X / card if there is no need to keep it running
      //request card switching
      if ((bb_status.appcount == 0) && (bb_config.stop_on_exit) && (bb_is_running(bb_status.x_pid) || (status_secondary() > 0))) {
        stop_secondary(1);
      }
    }

    /* Accept a connection. */
    optirun_socket_fd = socketAccept(&bb_status.bb_socket, SOCK_NOBLOCK);
    if (optirun_socket_fd >= 0) {
      bb_log(LOG_DEBUG, "Accepted new connection\n", optirun_socket_fd, bb_status.appcount);

      /* add to list of sockets */
      curr = malloc(sizeof (struct clientsocket));
      curr->sock = optirun_socket_fd;
      curr->inuse = 0;
      curr->next = 0;
      if (last == 0) {
        first = curr;
        last = curr;
      } else {
        last->next = curr;
        last = curr;
      }
    }

    /* loop through all connections, removing dead ones, receiving/sending data to the rest */
    curr = first;
    prev = 0;
    while (curr != 0) {
      if (curr->sock < 0) {
        //remove from list
        if (curr->inuse > 0) {
          bb_status.appcount--;
        }
        if (last == curr) {
          last = prev;
        }
        if (prev == 0) {
          first = curr->next;
          free(curr);
          curr = first;
        } else {
          prev->next = curr->next;
          free(curr);
          curr = prev->next;
        }
      } else {
        //active connection, handle it.
        handle_socket(curr);
        prev = curr;
        curr = curr->next;
      }
    }
  }//socket server loop

  /* loop through all connections, closing all of them */
  curr = first;
  prev = 0;
  while (curr != 0) {
    //close socket if not already closed
    if (curr->sock >= 0) {
      socketClose(&curr->sock);
    }
    //remove from list
    if (curr->inuse > 0) {
      bb_status.appcount--;
    }
    if (last == curr) {
      last = prev;
    }
    if (prev == 0) {
      first = curr->next;
      free(curr);
      curr = first;
    } else {
      prev->next = curr->next;
      free(curr);
      curr = prev->next;
    }
  }
}

int main(int argc, char* argv[]) {
  /* Setup signal handling before anything else */
  signal(SIGHUP, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGINT, handle_signal);
  signal(SIGQUIT, handle_signal);

  bb_init_log();

  check_secondary();
  init_config(argc, argv);

  /* Change GID and mask according to configuration */
  if ((bb_config.gid_name != 0) && (bb_config.gid_name[0] != 0)){
    bb_chgid();
  }

  bb_log(LOG_DEBUG, "%s version %s starting...\n", bb_status.program_name, GITVERSION);

  /* Daemonized if daemon flag is activated */
  if (bb_status.runmode == BB_RUN_DAEMON) {
    daemonize();
  }

  /* Initialize communication socket, enter main loop */
  bb_status.bb_socket = socketServer(bb_config.socket_path, SOCK_NOBLOCK);
  main_loop();
  unlink(bb_config.socket_path);
  stop_secondary(0); //stop X and/or, don't request card switch
  bb_closelog();
  bb_stop_all(); //stop any started processes that are left
  return (EXIT_SUCCESS);
}
