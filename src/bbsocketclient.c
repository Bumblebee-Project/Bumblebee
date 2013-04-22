/*
 * Copyright (c) 2012-2013, The Bumblebee Project
 * Author: Peter Lekensteyn <lekensteyn@gmail.com>
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
 * Functions for communicating with the daemon through a socket
 */

#include <stdio.h>
#include <string.h>
#include <sys/syslog.h>
#include "bbsocket.h"
#include "bbsocketclient.h"
#include "bbconfig.h"
#include "bblogger.h"

/**
 * Requests a key over the socket
 * @param key The key to be retrieved
 * @param target A pointer to a char to store the value in
 * @param max_len The maximum number of bytes to be written in target, must be
 * greater than 0
 * @return 0 on success, non-zero on failure
 */
int bbsocket_query(const char *key, char *target, size_t max_len) {
  char buff[BUFFER_SIZE];
  snprintf(buff, sizeof buff, "Query %s", key);
  if (!socketWrite(&bb_status.bb_socket, buff, strlen(buff) + 1)) {
    bb_log(LOG_DEBUG, "Write failed for query of %s\n", key);
    return 1;
  }
  while (bb_status.bb_socket != -1) {
    int r = socketRead(&bb_status.bb_socket, buff, sizeof (buff));
    if (r > 0) {
      ensureZeroTerminated(buff, r, sizeof (buff));
      if (strncmp("Value: ", buff, strlen("Value: "))) {
        bb_log(LOG_DEBUG, "Failed to query for %s: %s\n", key, buff);
        return 1;
      }
      strncpy(target, buff + strlen("Value: "), max_len);
      target[max_len - 1] = 0;
      /* remove trailing newline */
      if (strlen(target)) {
        target[strlen(target) - 1] = 0;
      }
      return 0;
    }
  }
  bb_log(LOG_DEBUG, "Read failed for query of %s\n", key);
  return 1;
}
