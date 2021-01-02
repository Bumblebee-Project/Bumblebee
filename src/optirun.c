/*
 * Copyright (c) 2011-2013, The Bumblebee Project
 * Author: Joaquín Ignacio Aramendía <samsagax@gmail.com>
 * Author: Jaron Viëtor AKA "Thulinma" <jaron@vietors.com>
 * Author: Lekensteyn <lekensteyn@gmail.com>
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

/* for strchrnul */
#define _GNU_SOURCE

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include "bbconfig.h"
#include "bbsocket.h"
#include "bbsocketclient.h"
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

/**
 * Prints the status of the Bumblebee server if available
 * @return EXIT_SUCCESS if the status is successfully retrieved,
 * EXIT_FAILURE otherwise
 */
static int report_daemon_status(void) {
  char buffer[BUFFER_SIZE];
  int r = snprintf(buffer, BUFFER_SIZE, "Status?");
  socketWrite(&bb_status.bb_socket, buffer, r + 1);
  while (bb_status.bb_socket != -1) {
    r = socketRead(&bb_status.bb_socket, buffer, BUFFER_SIZE);
    if (r > 0) {
      ensureZeroTerminated(buffer, r, BUFFER_SIZE);
      printf("Bumblebee status: %s\n", buffer);
      socketClose(&bb_status.bb_socket);
      return EXIT_SUCCESS;
    }
  }
  return EXIT_FAILURE;
}

/**
 * Runs a requested program if fallback mode was enabled
 * @param argv The program and param list to be executed
 * @return EXIT_FAILURE on failure and if fallback was disabled, -1 on failure
 * and if fallback was enabled and never on success
 */
static int run_fallback(char *argv[]) {
  if (bb_status.runmode == BB_RUN_APP && bb_config.fallback_start) {
    bb_log(LOG_WARNING, "The Bumblebee server was not available.\n");
    bb_run_exec(argv);
    bb_log(LOG_ERR, "Unable to start program in fallback mode.\n");
  }
  return EXIT_FAILURE;
}

static int check_virtualgl(void) {
  /* check if vglrun and vglclient exist */
  char *p = which_program("vglrun");
  int has_virtualgl = (p != NULL);
  free(p);
  p = which_program("vglclient");
  has_virtualgl = has_virtualgl && (p != NULL);
  free(p);
  return has_virtualgl;
}

static int run_virtualgl(int argc, char **argv) {
  //run vglclient if any method other than proxy is used
  if (strncmp(bb_config.vgl_compress, "proxy", BUFFER_SIZE) != 0) {
    char * vglclient_args[] = {
      "vglclient",
      "-detach",
      0
    };
    bb_run_fork(vglclient_args, 1);
  }
  /* number of options passed to --vgl-options */
  unsigned int vglrun_opts_count = 0;
  char *next_arg = bb_config.vglrun_options;
  /* read vglrun options only if there is an arguments list */
  if (next_arg && next_arg[0]) {
    do {
      ++vglrun_opts_count;
    } while ((next_arg = strchr(next_arg + 1, ' ')));
  }
  /* position of next option */
  unsigned int optno = 0;

  /* 7 for the first options, 1 for the -- and 1 for the trailing 0 */
  char ** vglrun_args = malloc(sizeof (char *) *
      (9 + vglrun_opts_count + argc - optind));
  vglrun_args[0] = "vglrun";
  vglrun_args[1] = "-c";
  vglrun_args[2] = bb_config.vgl_compress;
  vglrun_args[3] = "-d";
  vglrun_args[4] = bb_config.x_display;
  vglrun_args[5] = "-ld";
  vglrun_args[6] = bb_config.ld_path;
  optno = 7;

  next_arg = bb_config.vglrun_options;
  if (next_arg && next_arg[0]) {
    char *current_arg;
    do {
      current_arg = next_arg;
      next_arg = strchr(current_arg, ' ');
      /* cut the string if a space is found */
      if (next_arg) {
        *next_arg = 0;
        /* the next argument starts at the position after the space */
        next_arg++;
      }
      vglrun_args[optno++] = current_arg;
    } while (next_arg);
  }

  vglrun_args[optno++] = "--";
  int r;
  for (r = 0; r < argc - optind; r++) {
    vglrun_args[r + optno] = argv[optind + r];
  }
  vglrun_args[optno+=r] = 0;
  /* set envvar for better performance on some systems, but allow the
   * user for manually override */
  setenv("VGL_READBACK", "pbo", 0);
  int exitcode = bb_run_fork(vglrun_args, 0);
  free(vglrun_args);
  return exitcode;
}

static int check_primus(void) {
  /* check if a libGL.so.1 can be found in one of the primus paths */
  char *path = bb_config.primus_ld_path;
  char *libgl = malloc(strlen(path) + sizeof("/libGL.so.1") + 1);
  int libgl_found = 0;
  do { /* iterate over paths separated by : */
    char *p = strchrnul(path, ':');
    int part_len = p - path;
    if (part_len > 0) {
      memcpy(libgl, path, part_len);
      strcpy(libgl + part_len, "/libGL.so.1");
      if (access(libgl, R_OK) == 0) {
        libgl_found = 1;
        break;
      }
    }
    path = p;
  } while (*path++ == ':'); /* after check, move to part after ':' */
  free(libgl);
  return libgl_found;
}

static int run_primus(int argc, char **argv) {
  char **run_args = malloc(sizeof (char *) * (1 + argc - optind));
  int r;
  for (r = 0; r < argc - optind; r++) {
    run_args[r] = argv[optind + r];
  }
  run_args[r] = 0;

  /* primus starts the X server when needed, fixes long-standing fork issue */
  setenv("BUMBLEBEE_SOCKET", bb_config.socket_path, 1);

  /* set LD_LIBRARY_PATH to primus_ld_path plus ld_path plus current LD_LIBRARY_PATH */
  setenv("PRIMUS_DISPLAY", bb_config.x_display, 0);
  char *ldpath_cur = getenv("LD_LIBRARY_PATH");
  char *ldpath_new = malloc(strlen(bb_config.primus_ld_path) + 1 + strlen(bb_config.ld_path) + 1 +
        (ldpath_cur ? strlen(ldpath_cur) : 0) + 1);
  strcpy(ldpath_new, bb_config.primus_ld_path);
  if (bb_config.ld_path[0]) {
    strcat(ldpath_new, ":");
    strcat(ldpath_new, bb_config.ld_path);
  }
  if (ldpath_cur) {
    strcat(ldpath_new, ":");
    strcat(ldpath_new, ldpath_cur);
  }
  setenv("LD_LIBRARY_PATH", ldpath_new, 1);
  free(ldpath_new);

  /* set PRIMUS_libGLa */
  char *libgl_mesa = "/usr/$LIB/libGL.so.1:/usr/lib/$LIB/libGL.so.1:/usr/$LIB/mesa/libGL.so.1:/usr/lib/$LIB/mesa/libGL.so.1";
  if (bb_config.ld_path[0]) { /* build new library path for PRIMUS_libGLa */
    int libgl_size = strlen(bb_config.ld_path) + 1;
    { /* calculate additional memories for adding "/libGL.so.1" */
      char *p = bb_config.ld_path;
      do {
        p = strchr(p, ':');
        libgl_size += sizeof("/libGL.so.1") - 1;
      } while (p++);
    }
    char *libgl = malloc(libgl_size);
    { /* from library path A:B build A/libGL.so.1:B/libGL.so.1 */
      int pos = 0;
      char *path = bb_config.ld_path;
      do {
        char *p = strchrnul(path, ':');
        int part_len = p - path;
        if (part_len > 0) {
          memcpy(libgl + pos, path, part_len);
          pos += part_len;
          snprintf(libgl + pos, libgl_size - pos, "/libGL.so.1%c", *p);
          pos += sizeof("/libGL.so.1:") - 1;
        }
        path = p;
      } while (*path++ == ':'); /* after check, move to part after ':' */
    }
    /* override PRIMUS_libGLa because PRIMUS does not know our driver */
    setenv("PRIMUS_libGLa", libgl, 0);
    free(libgl);
  } else { /* no LibraryPath is set, assume OSS drivers */
    setenv("PRIMUS_libGLa", libgl_mesa, 0);
  }
  /* assume OSS drivers for primary display (Mesa for Intel) */
  setenv("PRIMUS_libGLd", libgl_mesa, 0);

  int exitcode = bb_run_fork(run_args, 0);
  free(run_args);
  return exitcode;
}

static int check_none(void) {
  return 1;
}

static int run_none(int argc, char **argv) {
  char **run_args = malloc(sizeof (char *) * (1 + argc - optind));
  int r;
  for (r = 0; r < argc - optind; r++) {
    run_args[r] = argv[optind + r];
  }
  run_args[r] = 0;

  if (bb_config.ld_path[0]) {
    char *ldpath_cur = getenv("LD_LIBRARY_PATH");
    char *ldpath_new = malloc(strlen(bb_config.ld_path) + 1 + (ldpath_cur ? strlen(ldpath_cur) : 0) + 1);
    strcpy(ldpath_new, bb_config.ld_path);
    if (ldpath_cur) {
      strcat(ldpath_new, ":");
      strcat(ldpath_new, ldpath_cur);
    }
    setenv("LD_LIBRARY_PATH", ldpath_new, 1);
    free(ldpath_new);
  }

  int exitcode = bb_run_fork(run_args, 0);
  free(run_args);
  return exitcode;
}

struct optirun_bridge {
  const char *name;
  int (*check_availability)(void);
  int (*run)(int argc, char **argv);
};

static struct optirun_bridge backends[] = {
  {"primus", check_primus, run_primus},
  {"virtualgl", check_virtualgl, run_virtualgl},
  {"none", check_none, run_none}, // keep last
  {NULL, NULL, NULL}
};

/**
 * Starts a program with Bumblebee if possible
 *
 * @param argc The number of arguments
 * @param argv The values of arguments
 * @return The exitcode of the program on success, EXIT_FAILURE if the program
 * could not be started
 */
static int run_app(int argc, char *argv[]) {
  int exitcode = EXIT_FAILURE;
  char buffer[BUFFER_SIZE];
  int r;
  int ranapp = 0;

  struct optirun_bridge *back = backends;
  if (!strcmp(bb_config.optirun_bridge, "auto")) {
    while (back->name && !back->check_availability()) ++back;
    if (!back->name || !strcmp(back->name, "none")) {
      bb_log(LOG_ERR, "No bridge found. Try installing primus or virtualgl.\n");
      goto out;
    }
    bb_log(LOG_DEBUG, "Using auto-detected bridge %s\n", back->name);
  } else {
    while (back->name && strcmp(bb_config.optirun_bridge, back->name)) ++back;
    if (!back->name) {
      bb_log(LOG_ERR, "Unknown accel/display bridge: %s\n", bb_config.optirun_bridge);
      goto out;
    }
    if (!back->check_availability()) {
      bb_log(LOG_ERR, "Accel/display bridge %s is not installed.\n", back->name);
      goto out;
    }
  }

  r = snprintf(buffer, BUFFER_SIZE, "Connect %s", bb_config.no_xorg ? "NoX" : "");
  socketWrite(&bb_status.bb_socket, buffer, r + 1);
  while (bb_status.bb_socket != -1) {
    r = socketRead(&bb_status.bb_socket, buffer, BUFFER_SIZE);
    if (r > 0) {
      r = ensureZeroTerminated(buffer, r, BUFFER_SIZE);
      bb_log(LOG_INFO, "Response: %s\n", buffer);
      switch (buffer[0]) {
        case 'N': //No, run normally.
          // buffer should contain "No, ..." or "No - ..."
          bb_log(LOG_ERR, "Cannot access secondary GPU%s\n", r > 2 ? buffer+2 : "");
          socketClose(&bb_status.bb_socket);
          if (!bb_config.fallback_start) {
            bb_log(LOG_ERR, "Aborting because fallback start is disabled.\n");
          }
          break;
        case 'Y': //Yes, run through vglrun
          bb_log(LOG_INFO, "Running application using %s.\n", back->name);
          ranapp = 1;
          exitcode = back->run(argc, argv);
          socketClose(&bb_status.bb_socket);
          break;
        default: //Something went wrong - output and exit.
          bb_log(LOG_ERR, "Problem: %s\n", buffer);
          socketClose(&bb_status.bb_socket);
          break;
      }
    }
  }
out:
  if (!ranapp) {
    exitcode = run_fallback(argv + optind);
  }
  return exitcode;
}

/**
 * Returns the option string for this program
 * @return An option string which can be used for getopt
 */
const char *bbconfig_get_optstr(void) {
  return BBCONFIG_COMMON_OPTSTR "c:b:";
}

/**
 * Returns the long options for this program
 * @return A option struct which can be used for getopt_long
 */
const struct option *bbconfig_get_lopts(void) {
  static struct option longOpts[] = {
    {"failsafe", 0, 0, OPT_FAILSAFE},
    {"no-failsafe", 0, 0, OPT_NO_FAILSAFE},
    {"no-xorg", 0, 0, OPT_NO_XORG},
    {"bridge", 1, 0, 'b'},
    {"vgl-compress", 1, 0, 'c'},
    {"vgl-options", 1, 0, OPT_VGL_OPTIONS},
    {"primus-ldpath", 1, 0, OPT_PRIMUS_LD_PATH},
    {"status", 0, 0, OPT_STATUS},
    BBCONFIG_COMMON_LOPTS
  };
  return longOpts;
}

/**
 * Parses local command line options
 * @param opt The short option
 * @param value Value for the option if any
 * @return 1 if the option has been processed, 0 otherwise
 */
int bbconfig_parse_options(int opt, char *value) {
  switch (opt) {
    case 'b':/* accel/display bridge, e.g. virtualgl or primus */
      set_string_value(&bb_config.optirun_bridge, value);
      break;
    case 'c'://vglclient method
      set_string_value(&bb_config.vgl_compress, value);
      break;
    case OPT_FAILSAFE:
      bb_config.fallback_start = 1;
      break;
    case OPT_NO_FAILSAFE:
      bb_config.fallback_start = 0;
      break;
    case OPT_NO_XORG:
      bb_config.no_xorg = 1;
      set_string_value(&bb_config.optirun_bridge, "none");
      break;
    case OPT_VGL_OPTIONS:
      set_string_value(&bb_config.vglrun_options, value);
      break;
    case OPT_PRIMUS_LD_PATH:
      set_string_value(&bb_config.primus_ld_path, value);
      break;
    case OPT_STATUS:
      bb_status.runmode = BB_RUN_STATUS;
      break;
    default:
      /* no options parsed */
      return 0;
  }
  return 1;
}

int main(int argc, char *argv[]) {
  int exitcode = EXIT_FAILURE;

  init_early_config(argv, BB_RUN_APP);

  /* Setup signal handling before anything else */
  signal(SIGHUP, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGINT, handle_signal);
  signal(SIGQUIT, handle_signal);

  bb_init_log();

  /* Initializing configuration */
  init_config();
  bbconfig_parse_opts(argc, argv, PARSE_STAGE_PRECONF);
  GKeyFile *bbcfg = bbconfig_parse_conf();
  if (bbcfg) g_key_file_free(bbcfg);

  /* Connect to listening daemon */
  bb_status.bb_socket = socketConnect(bb_config.socket_path, SOCK_BLOCK);
  if (bb_status.bb_socket < 0) {
    bb_log(LOG_ERR, "Could not connect to bumblebee daemon - is it running?\n");
    run_fallback(argv + optind);
    bb_closelog();
    return exitcode;
  }

  free_and_set_value(&bb_config.ld_path, malloc(BUFFER_SIZE));
  if (bbsocket_query("LibraryPath", bb_config.ld_path, BUFFER_SIZE)) {
    bb_log(LOG_ERR, "Failed to retrieve LibraryPath setting.\n");
    return EXIT_FAILURE;
  }
  free_and_set_value(&bb_config.x_display, malloc(BUFFER_SIZE));
  if (bbsocket_query("VirtualDisplay", bb_config.x_display, BUFFER_SIZE)) {
    bb_log(LOG_ERR, "Failed to retrieve VirtualDisplay setting.\n");
    return EXIT_FAILURE;
  }

  /* parse remaining common and optirun-specific options */
  bbconfig_parse_opts(argc, argv, PARSE_STAGE_OTHER);
  bb_log(LOG_DEBUG, "%s version %s starting...\n", "optirun", GITVERSION);
  config_dump();

  /* Request status */
  if (bb_status.runmode == BB_RUN_STATUS) {
    exitcode = report_daemon_status();
  }

  /* Run given application */
  if (bb_status.runmode == BB_RUN_APP) {
    if (optind >= argc) {
      bb_log(LOG_ERR, "Missing argument: application to run\n");
      print_usage(EXIT_FAILURE);
    } else {
      exitcode = run_app(argc, argv);
    }
  }

  bb_closelog();
  bb_stop_all(); //stop any started processes that are left
  return exitcode;
}
