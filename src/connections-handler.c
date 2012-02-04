/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Joaquín Ignacio Aramendía <samsagax@gmail.com>
 * Author: Jaron Viëtor AKA "Thulinma" <jaron@vietors.com>
 *         Peter Lekensteyn <lekensteyn@gmail.com>
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

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bbconfig.h"
#include "bbsocket.h"
#include "bblogger.h"
#include "bbsecondary.h"
#include "bbrun.h"
#include "switch/switching.h"
#include "connections-handler.h"
#include "dbus/dbus.h"

/* the last client in the connected clients list */
static struct clientsocket *last;

static void handle_socket(struct clientsocket * C);

/* Handles all connections and cleanup.
 * It aborts the main loop if there are any problems with the listening socket.
 */
gboolean handle_connection(gpointer data) {
  if (bb_status.bb_socket == -1) {
    bb_log(LOG_DEBUG, "Stopping mainloop because socket is closed.\n");
    g_main_loop_quit((GMainLoop *)data);
    return FALSE;
  }
  struct clientsocket *client;
  int optirun_socket_fd;
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
    if (last) {
      last->next = client;
    }
    last = client;
  }

  //check the X output pipe, if open
  check_xorg_pipe();

  /* loop through all connections, removing dead ones, receiving/sending data to the rest */
  struct clientsocket *next_iter;
  for (client = last; client; client = next_iter) {
    /* set the next client here because client may be free()'d */
    next_iter = client->prev;
    if (client->sock < 0) {
      //remove from list
      if (client->inuse > 0) {
        bb_dbus_set_clients_count(--bb_status.appcount);
        //stop X / card if there is no need to keep it running
        if ((bb_status.appcount == 0) && (bb_config.stop_on_exit)) {
          stop_secondary();
        }
      }
      if (client->next) {
        client->next->prev = client->prev;
      } else {
        last = client->prev;
      }
      if (client->prev) {
        client->prev->next = client->next;
      }
      free(client);
    } else {
      //active connection, handle it.
      handle_socket(client);
    }
  }
  return TRUE;
}//socket server loop

void connections_fini(void) {
  /* loop through all connections, closing all of them */
  struct clientsocket *client = last;
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
  if (bb_status.appcount != 0) {
    bb_log(LOG_WARNING, "appcount = %i (should be 0)\n", bb_status.appcount);
  }
}


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
            char *card_status;
            switch (switch_status()) {
              case SWITCH_OFF:
                card_status = "off";
                break;
              case SWITCH_ON:
                card_status = "on";
                break;
              default:
                /* no PM available, assume it's on */
                card_status = "likely on";
                break;
            }
            r = snprintf(buffer, BUFFER_SIZE, "Ready (%s). X inactive. Discrete"
                    " video card is %s.\n", GITVERSION, card_status);
          }
        }
        /* don't rely on result of snprintf, instead calculate length including
         * null byte. We assume a succesful write */
        socketWrite(&C->sock, buffer, strlen(buffer) + 1);
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
            bb_dbus_set_clients_count(++bb_status.appcount);
          }
        } else {
          if (bb_status.errors[0] != 0) {
            r = snprintf(buffer, BUFFER_SIZE, "No - error: %s\n", bb_status.errors);
          } else {
            r = snprintf(buffer, BUFFER_SIZE, "No, secondary X is not active.\n");
          }
        }
        /* don't rely on result of snprintf, instead calculate length including
         * null byte. We assume a succesful write */
        socketWrite(&C->sock, buffer, strlen(buffer) + 1);
        break;
      case 'D'://done, close the socket.
        socketClose(&C->sock);
        break;
      case 'Q': /* query for configuration details */
        /* required since labels can only be attached on statements */;
        char *conf_key = strchr(buffer, ' ');
        if (conf_key) {
          conf_key++;
          if (strcmp(conf_key, "VirtualDisplay") == 0) {
            snprintf(buffer, BUFFER_SIZE, "Value: %s\n", bb_config.x_display);
          } else if (strcmp(conf_key, "LibraryPath") == 0) {
            snprintf(buffer, BUFFER_SIZE, "Value: %s\n", bb_config.ld_path);
          } else if (strcmp(conf_key, "Driver") == 0) {
            /* note: this is not the auto-detected value, but the actual one */
            snprintf(buffer, BUFFER_SIZE, "Value: %s\n", bb_config.driver);
          } else {
            snprintf(buffer, BUFFER_SIZE, "Unknown key requested.\n");
          }
        } else {
          snprintf(buffer, BUFFER_SIZE, "Error: invalid protocol message.\n");
        }
        socketWrite(&C->sock, buffer, strlen(buffer) + 1);
        break;
      default:
        bb_log(LOG_WARNING, "Unhandled message received: %*s\n", r, buffer);
        break;
    }
  }
}
