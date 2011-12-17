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
 * bblogger.c: loggin functions for bumblebee daemon and client
 */

#include "bblogger.h"
#include "bbglobals.h"

/**
 * Initialize log capabilities. Return 0 on success
 */
int bb_init_log() {
    /*  Open Logggin mechanism based on configuration */
    if (bb_status.is_daemonized) {
        openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);
    } else {
    }
    /*  Should end with no error by now */
    return 0;
}

/**
 * Log a message to the current log mechanism.
 * Try to keep log messages less than 80 characters.
 */
void bb_log(int priority, char* msg_format, ...) {
  switch (priority) {
    case LOG_ERR:
      if (bb_status.verbosity < VERB_ERR){return;}
      break;
    case LOG_DEBUG:
      if (bb_status.verbosity < VERB_DEBUG){return;}
      break;
    case LOG_WARNING:
      if (bb_status.verbosity < VERB_WARN){return;}
      break;
    default:
      if (bb_status.verbosity < VERB_INFO){return;}
      break;
  }

  va_list args;
  va_start(args, msg_format);
  if (bb_status.is_daemonized) {
      vsyslog(priority, msg_format, args);
  } else {
      char* fullmsg_fmt = malloc(BUFFER_SIZE);
      switch (priority) {
          case LOG_ERR:
              fullmsg_fmt = strcpy(fullmsg_fmt, "[ERROR]");
              break;
          case LOG_DEBUG:
              fullmsg_fmt = strcpy(fullmsg_fmt, "[DEBUG]");
              break;
          case LOG_WARNING:
              fullmsg_fmt = strcpy(fullmsg_fmt, "[WARN]");
              break;
          default:
              fullmsg_fmt = strcpy(fullmsg_fmt, "[INFO]");
      }
      fullmsg_fmt = strcat(fullmsg_fmt, msg_format);
      vfprintf(stderr, fullmsg_fmt, args);
      free(fullmsg_fmt);
  }
  va_end(args);
}

/**
 * Close logging mechanism
 */
void bb_closelog() {
    if (bb_status.is_daemonized) {
        closelog();
    } else {
    }
}

