/*
 * Copyright (c) 2011-2013, The Bumblebee Project
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "bblogger.h"
#include "bbconfig.h"

char x_output_buffer[512]; /* Xorg output buffer */
int x_buffer_pos = 0;/* Xorg output buffer position */


/**
 * Initialize log capabilities. Return 0 on success
 */
int bb_init_log(void) {
  /*  Open Logggin mechanism based on configuration */
  if (bb_status.use_syslog) {
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
      if (bb_status.verbosity < VERB_ERR) {
        return;
      }
      break;
    case LOG_WARNING:
      if (bb_status.verbosity < VERB_WARN) {
        return;
      }
      break;
    case LOG_NOTICE:
      if (bb_status.verbosity < VERB_NOTICE) {
        return;
      }
      break;
    case LOG_INFO:
      if (bb_status.verbosity < VERB_INFO) {
        return;
      }
      break;
    case LOG_DEBUG:
      if (bb_status.verbosity < VERB_DEBUG) {
        return;
      }
      break;
    default:
      /* unspecified log level, always log it unless verbosity is NONE */
      if (bb_status.verbosity == VERB_NONE) {
        return;
      }
      break;
  }

  va_list args;
  va_start(args, msg_format);
  if (bb_status.use_syslog) {
    vsyslog(priority, msg_format, args);
  } else {
    char* fullmsg_fmt = malloc(BUFFER_SIZE + 8);
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
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
    fullmsg_fmt = strncat(fullmsg_fmt, msg_format, BUFFER_SIZE);
    fprintf(stderr, "[%5llu.%06lu] ", (long long)tp.tv_sec, tp.tv_nsec / 1000);
    vfprintf(stderr, fullmsg_fmt, args);
    free(fullmsg_fmt);
  }
  va_end(args);
}

/**
 * Close logging mechanism
 */
void bb_closelog(void) {
  if (bb_status.use_syslog) {
    closelog();
  }
}

/** Parses a single null-terminated string of Xorg output.
 * Will call bb_log appropiately.
 */
static void parse_xorg_output(char * string){
  int prio = LOG_DEBUG;/* most lines are debug messages */
  char * valid = 0; /* Helper for finding correct ConnectedMonitor setting */
  char * valid_end = 0; /* Helper for finding correct ConnectedMonitor setting */
  /* message to be logged with set_bb_error */
  char error_buffer[strlen("[XORG] ") + sizeof (x_output_buffer)];

  /* don't log an empty line or a line with a single whitespace */
  if (string[0] == 0 || (string[1] == 0 && isspace(string[0]))) {
    return;
  }

  /* Error lines are errors. */
  if (strncmp(string, "(EE)", 4) == 0){
    if (strstr(string, "Failed to load module \"kbd\"") ||
            strstr(string, "No input driver matching")) {
      /* non-fatal errors */
      prio = LOG_DEBUG;
    } else {
      /* prefix with [XORG] */
      snprintf(error_buffer, sizeof error_buffer, "[XORG] %s", string);
      set_bb_error(error_buffer);//set as error
      /* errors are handled seperately from the rest - return */
      return;
    }
  }

  /* Warning lines are warnings. */
  if (strncmp(string, "(WW)", 4) == 0){
    prio = LOG_WARNING;
    /* recognize some of the less useful warnings, degrade them to LOG_DEBUG level. */
    if (
            /* nouveau: warning about no outputs being found connected */
            strstr(string, "trying again") ||
            /* nouveau: warning for set resolution with no screen attached */
            strstr(string, "initial framebuffer") ||
            /* X: no keyboard/mouse warning */
            strstr(string, "looking for one") ||
            /* nvidia: cannot read EDID warning */
            strstr(string, "EDID") ||
            /* fonts directory that cannot be found */
            strstr(string, "The directory \"") ||
            /* kbd module that is trying to get loaded */
            strstr(string, "couldn't open module kbd") ||
            /* we're not interested in input drivers */
            strstr(string, "No input driver matching")) {
      prio = LOG_DEBUG;
    } else if (strstr(string, "valid display devices are")) {
      /* Recognize nvidia complaining about ConnectedMonitor setting */
      valid = strchr(string, '\'');//find the '-character
      if (valid){
        char last_chr = 0;
        valid_end = ++valid;/* advance valid one character, start searching for end */
        while (valid_end[0] != 0){
          last_chr = valid_end[0];
          if (last_chr == '\'' || last_chr == ',' || last_chr == ' ') {
            valid_end[0] = 0;
            break;
          }
          valid_end++;
        }
        set_bb_error(0); /* Clear error message, we want to override it even though it is not first */
        snprintf(error_buffer, sizeof error_buffer, "You need to change the"
                " ConnectedMonitor setting in %s to %s",
                bb_config.x_conf_file, valid);
        set_bb_error(error_buffer);//set as error
        /* Restore the string for logging purposes */
        valid_end[0] = last_chr;
      }
    }
  }
  
  /* do the actual logging */
  bb_log(prio, "[XORG] %s\n", string);
}

/** Will check the xorg output pipe and parse any waiting messages.
 * Doesn't take any parameters and doesn't return anything.
 */
void check_xorg_pipe(void){
  if (bb_status.x_pipe[0] == -1){return;}
  int repeat;

  do{
    repeat = 0;
    /* attempt to read at most the entire buffer full. */
    int r = read(bb_status.x_pipe[0], x_output_buffer + x_buffer_pos,
            sizeof (x_output_buffer) - x_buffer_pos - 1);
    if (r > 0){
      x_buffer_pos += r;
      /* append a null byte to close the string */
      x_output_buffer[x_buffer_pos] = 0;
      if (x_buffer_pos == sizeof (x_output_buffer) - 1) {
        /* line / buffer is full, process the remaining buffer the next round */
        repeat = 1;
      }
    } else {
      if (r == 0 || (errno != EAGAIN && r == -1)){
        /* the pipe is closed/invalid. Clean up. */
        if (bb_status.x_pipe[0] != -1){close(bb_status.x_pipe[0]); bb_status.x_pipe[0] = -1;}
        if (bb_status.x_pipe[1] != -1){close(bb_status.x_pipe[1]); bb_status.x_pipe[1] = -1;}
      }
    }
    /* while x_buffer_pos>0 and a \n is in the buffer, parse.
     * if buffer is full, parse also. */
    while (x_buffer_pos > 0){
      char * foundnewline = strchr(x_output_buffer, '\n');
      if (!foundnewline || foundnewline-x_output_buffer > x_buffer_pos){
        /* cancel search if no newline, try again later
         * except if buffer is full, then parse */
        if (x_buffer_pos == sizeof (x_output_buffer) - 1) {
          parse_xorg_output(x_output_buffer);
          x_buffer_pos = 0;
        }
        break;
      }
      foundnewline[0] = 0;/* convert newline to null byte */
      parse_xorg_output(x_output_buffer);/* parse the line */
      char *next_part = foundnewline + 1; /* begin of next line */
      int size = next_part - x_output_buffer;
      x_buffer_pos -= size;/* cut the parsed part from the buffer size */
      if (x_buffer_pos > 0){/* move the unparsed part left, if any */
        memmove(x_output_buffer, next_part, x_buffer_pos);
      }
    }
  }while (repeat);
}/* check_xorg_pipe */
