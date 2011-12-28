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

#include <stdlib.h>
#include <sys/stat.h>
#include <grp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bbconfig.h"
#include "bbsocket.h"
#include "bblogger.h"
#include "bbsecondary.h"
#include "bbrun.h"
#include "pci.h"

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
  umask(027);
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
  struct clientsocket * prev;
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
  struct clientsocket *client;
  struct clientsocket *last = 0;// the last client

  bb_log(LOG_INFO, "Started main loop\n");
  /* Listen for Optirun conections and act accordingly */
  while (bb_status.bb_socket != -1) {
    usleep(100000); //sleep 100ms to prevent 100% CPU time usage

    /* Accept a connection. */
    optirun_socket_fd = socketAccept(&bb_status.bb_socket, SOCK_NOBLOCK);
    if (optirun_socket_fd >= 0) {
      bb_log(LOG_DEBUG, "Accepted new connection\n", optirun_socket_fd, bb_status.appcount);

      /* add to list of sockets */
      client = malloc(sizeof (struct clientsocket));
      client->sock = optirun_socket_fd;
      client->inuse = 0;
      client->prev = last;
      client->next = 0;
      last = client;
    }

    /* loop through all connections, removing dead ones, receiving/sending data to the rest */
    struct clientsocket *next_iter;
    for (client = last; client; client = next_iter) {
      /* set the next client here because client may be free()'d */
      next_iter = client->prev;
      if (client->sock < 0) {
        //remove from list
        if (client->inuse > 0) {
          bb_status.appcount--;
          //stop X / card if there is no need to keep it running
          if ((bb_status.appcount == 0) && (bb_config.stop_on_exit)) {
            stop_secondary();
          }
        }
        if (client->next) {
          client->next = client->prev;
        } else {
          last = client->prev;
        }
        if (client->prev) {
          client->prev = client->next;
        }
        free(client);
      } else {
        //active connection, handle it.
        handle_socket(client);
      }
    }
  }//socket server loop

  /* loop through all connections, closing all of them */
  client = last;
  while (client) {
    //close socket if not already closed
    if (client->sock >= 0) {
      socketClose(&client->sock);
    }
    //remove from list
    if (client->inuse > 0) {
      bb_status.appcount--;
    }
    // change the client here because after free() there is no way to know prev
    last = client;
    client = client->prev;
    free(last);
  }
}

int main(int argc, char* argv[]) {
  bb_status.bb_socket = -1;//initialize the main socket to -1

  /* Setup signal handling before anything else. Note that messages are not
   * shown until init_config has set bb_status.verbosity
   */
  signal(SIGHUP, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGINT, handle_signal);
  signal(SIGQUIT, handle_signal);

  bb_init_log();

  /* first load the config to make the logging verbosity level available */
  init_config(argc, argv);
  pci_bus_id = pci_find_gfx_by_vendor(PCI_VENDOR_ID_NVIDIA);
  check_secondary();
  /* dump the config after detecting the driver */
  config_dump();

  /* Change GID and mask according to configuration */
  if ((bb_config.gid_name != 0) && (bb_config.gid_name[0] != 0)) {
    bb_chgid();
  }

  bb_log(LOG_DEBUG, "%s version %s starting...\n", bb_status.program_name, GITVERSION);

  /* Daemonized if daemon flag is activated */
  if (bb_status.runmode == BB_RUN_DAEMON) {
    daemonize();
  }

  /* Initialize communication socket, enter main loop */
  bb_status.bb_socket = socketServer(bb_config.socket_path, SOCK_NOBLOCK);
  stop_secondary(); //turn off card, nobody is connected right now.
  main_loop();
  unlink(bb_config.socket_path);
  bb_status.runmode = BB_RUN_EXIT; //make sure all methods understand we are shutting down
  if (bb_config.card_shutdown_state) {
    //if shutdown state = 1, turn on card
    start_secondary();
  } else {
    //if shutdown state = 0, turn off card
    stop_secondary();
  }
  bb_closelog();
  bb_stop_all(); //stop any started processes that are left
  return (EXIT_SUCCESS);
}
