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
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "bbconfig.h"
#include "bbsocket.h"
#include "bblogger.h"
#include "bbrun.h"

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

static void report_daemon_status(void) {
  char buffer[BUFFER_SIZE];
  int r = snprintf(buffer, BUFFER_SIZE, "Status?");
  socketWrite(&bb_status.bb_socket, buffer, r);
  while (bb_status.bb_socket != -1) {
    r = socketRead(&bb_status.bb_socket, buffer, BUFFER_SIZE);
    if (r > 0) {
      printf("Bumblebee status: %*s\n", r, buffer);
      socketClose(&bb_status.bb_socket);
    }
  }
}

/**
 * Runs a requested program if fallback mode was enabled
 * @param argv The program and param list to be executed
 */
static void run_fallback(char *argv[]) {
  if (bb_status.runmode == BB_RUN_APP && bb_config.fallback_start) {
    bb_log(LOG_WARNING, "The Bumblebee server was not available.\n");
    bb_run_exec(argv);
  }
}

static void run_app(int argc, char *argv[]) {
  char buffer[BUFFER_SIZE];
  int r;
  int ranapp = 0;
  r = snprintf(buffer, BUFFER_SIZE, "Checking availability...");
  socketWrite(&bb_status.bb_socket, buffer, r);
  while (bb_status.bb_socket != -1) {
    r = socketRead(&bb_status.bb_socket, buffer, BUFFER_SIZE);
    if (r > 0) {
      bb_log(LOG_INFO, "Response: %*s\n", r, buffer);
      switch (buffer[0]) {
        case 'N': //No, run normally.
          socketClose(&bb_status.bb_socket);
          if (!bb_config.fallback_start) {
            bb_log(LOG_ERR, "Cannot access secondary GPU. Aborting.\n");
          }
          break;
        case 'Y': //Yes, run through vglrun
          bb_log(LOG_INFO, "Running application through vglrun.\n");
          ranapp = 1;
          //run vglclient if any method other than proxy is used
          if (strncmp(bb_config.vgl_compress, "proxy", BUFFER_SIZE) != 0) {
            char * vglclient_args[] = {
              "vglclient",
              "-detach",
              0
            };
            bb_run_fork(vglclient_args);
          }
          char ** vglrun_args = malloc(sizeof (char *) * (9 + argc - optind));
          vglrun_args[0] = "vglrun";
          vglrun_args[1] = "-c";
          vglrun_args[2] = bb_config.vgl_compress;
          vglrun_args[3] = "-d";
          vglrun_args[4] = bb_config.x_display;
          vglrun_args[5] = "-ld";
          vglrun_args[6] = bb_config.ld_path;
          vglrun_args[7] = "--";
          for (r = 0; r < argc - optind; r++) {
            vglrun_args[8 + r] = argv[optind + r];
          }
          vglrun_args[8 + r] = 0;
          bb_run_fork_wait(vglrun_args, 0);
          free(vglrun_args);
          socketClose(&bb_status.bb_socket);
          break;
        default: //Something went wrong - output and exit.
          bb_log(LOG_ERR, "Problem: %*s\n", r, buffer);
          socketClose(&bb_status.bb_socket);
          break;
      }
    }
  }
  if (!ranapp) {
    run_fallback(argv + optind);
  }
}

int main(int argc, char *argv[]) {

  /* Setup signal handling before anything else */
  signal(SIGHUP, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGINT, handle_signal);
  signal(SIGQUIT, handle_signal);

  bb_init_log();

  /* Initializing configuration */
  init_config(argc, argv);
  config_dump();

  /* set runmode depending on leftover arguments */
  if (optind >= argc) {
    bb_status.runmode = BB_RUN_STATUS;
  } else {
    bb_status.runmode = BB_RUN_APP;
  }

  bb_log(LOG_DEBUG, "%s version %s starting...\n", bb_status.program_name, GITVERSION);

  /* Connect to listening daemon */
  bb_status.bb_socket = socketConnect(bb_config.socket_path, SOCK_NOBLOCK);
  if (bb_status.bb_socket < 0) {
    bb_log(LOG_ERR, "Could not connect to bumblebee daemon - is it running?\n");
    run_fallback(argv + optind);
    bb_closelog();
    return EXIT_FAILURE;
  }

  /* Request status */
  if (bb_status.runmode == BB_RUN_STATUS) {
    report_daemon_status();
  }

  /* Run given application */
  if (bb_status.runmode == BB_RUN_APP) {
    run_app(argc, argv);
  }

  bb_closelog();
  bb_stop_all(); //stop any started processes that are left
  return (EXIT_SUCCESS);
}
