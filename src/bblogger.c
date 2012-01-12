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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include "bblogger.h"
#include "bbconfig.h"

#define X_BUFFER_SIZE 512
char x_output_buffer[X_BUFFER_SIZE+1]; //Xorg output buffer
int x_buffer_pos = 0;//Xorg output buffer position


/**
 * Initialize log capabilities. Return 0 on success
 */
int bb_init_log(void) {
  /*  Open Logggin mechanism based on configuration */
  if (bb_status.runmode == BB_RUN_DAEMON) {
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
    case LOG_DEBUG:
      if (bb_status.verbosity < VERB_DEBUG) {
        return;
      }
      break;
    case LOG_WARNING:
      if (bb_status.verbosity < VERB_WARN) {
        return;
      }
      break;
    default:
      if (bb_status.verbosity < VERB_INFO) {
        return;
      }
      break;
  }

  va_list args;
  va_start(args, msg_format);
  if (bb_status.runmode == BB_RUN_DAEMON) {
    vsyslog(priority, msg_format, args);
  } else {
    char* fullmsg_fmt = malloc(BUFFER_SIZE + 8);
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
    vfprintf(stderr, fullmsg_fmt, args);
    free(fullmsg_fmt);
  }
  va_end(args);
}

/**
 * Close logging mechanism
 */
void bb_closelog(void) {
  if (bb_status.runmode == BB_RUN_DAEMON) {
    closelog();
  }
}

/** Parses a single null-terminated string of Xorg output.
 * Will call bb_log appropiately.
 */
static void parse_xorg_output(char * string){
  int prio = LOG_DEBUG;//most lines are debug messages
  
  //Error lines are errors.
  if (strstr(string, "(EE)")){
    //ignore the line with all types of lines
    if (strstr(string, "(WW)")){return;}
    //prefix with [XORG]
    char error_buffer[X_BUFFER_SIZE+8];
    snprintf(error_buffer, X_BUFFER_SIZE+8, "[XORG] %s", string);
    set_bb_error(error_buffer);//set as error
    //errors are handled seperately from the rest - return
    return;
  }
  
  //Warning lines are warnings.
  if (strstr(string, "(WW)")){prio = LOG_WARNING;}
  /// \todo Convert useless/meaningless warnings to LOG_INFO
  
  //do the actual logging
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
    //attempt to read at most the entire buffer full.
    int r = read(bb_status.x_pipe[0], x_output_buffer+x_buffer_pos, X_BUFFER_SIZE-x_buffer_pos);
    if (r > 0){
      x_buffer_pos += r;
      if (x_buffer_pos == X_BUFFER_SIZE){repeat = 1;}//ensure we read all we can
    }else{
      if (r == 0 || (errno != EAGAIN && r == -1)){
        //the pipe is closed/invalid. Clean up.
        if (bb_status.x_pipe[0] != -1){close(bb_status.x_pipe[0]); bb_status.x_pipe[0] = -1;}
        if (bb_status.x_pipe[1] != -1){close(bb_status.x_pipe[1]); bb_status.x_pipe[1] = -1;}
      }
    }
    //while x_buffer_pos>0 and a \n is in the buffer, parse.
    //if buffer is full, parse also.
    while (x_buffer_pos > 0){
      x_output_buffer[X_BUFFER_SIZE] = 0;//make sure there's a terminating null byte
      char * foundnewline = strchr(x_output_buffer, '\n');
      if (!foundnewline || foundnewline-x_output_buffer > x_buffer_pos){
        //cancel search if no newline, try again later
        //except if buffer is full, then parse
        if (x_buffer_pos == X_BUFFER_SIZE){
          parse_xorg_output(x_output_buffer);
          x_buffer_pos = 0;
        }
        break;
      }
      foundnewline[0] = 0;//convert newline to null byte
      parse_xorg_output(x_output_buffer);//parse the line
      int size = foundnewline - x_output_buffer + 1;
      x_buffer_pos -= size;//cut the parsed part from the buffer size
      if (x_buffer_pos > 0){//move the unparsed part left, if any
        memmove(x_output_buffer, foundnewline + 1, x_buffer_pos);
      }
    }
  }while(repeat);
}//check_xorg_pipe
