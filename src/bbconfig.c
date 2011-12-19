/*
 * Copyright (c) 2011, The Bumblebee Project
 * Author: Joaquín Ignacio Aramendía samsagax@gmail.com
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
 * bbconfig.c: Bumblebee configuration file handler
 */

#include "bbconfig.h"
#include "bblogger.h"
#include "config.h"
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

struct bb_status_struct bb_status;
struct bb_config_struct bb_config;

struct bb_key_value {
  char key[BUFFER_SIZE];
  char value[BUFFER_SIZE];
};

/** 
 * Takes a line and returns a key-value pair
 *
 * @param line String to be broken into a key-value pair
 */
static struct bb_key_value bb_get_key_value(const char* line) {
  struct bb_key_value kvpair;
  if (EOF == sscanf(line, "%[^=]=%[^\n]", kvpair.key, kvpair.value)) {
    int err_val = errno;
    printf("Error parsing configuration file: %s\n", strerror(err_val));
  }
  return kvpair;
}

/**
 * Strips all whitespaces from a string
 *
 * @param str String to be cleared of whitespaces
 */
static void stripws(char* str) {
  char *orig = str;
  char *stripped = str;
  orig = str;
  while (*orig != 0) {
    if (isspace(*orig)) {
      ++orig;
    } else {
      *stripped++ = *orig++;
    }
  }
  str = stripped;
}

/**
 *  Read the configuration file.
 *
 *  @return 0 on success. 
 */
static int read_configuration( void ) {

  FILE *cf = fopen(bb_config.bb_conf_file, "r");
  if (cf == (NULL)) { /* An error ocurred */
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
        bb_log(LOG_ERR, "Error in config file: %s", strerror(err_num));
    }
  } else {
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof line, cf) != NULL) {
      stripws(line);
      /* Ignore empty lines and comments */
      if ((line[0] != '#') && (line[0] != '\n')) {
        /* Parse configuration based on the run mode */
        struct bb_key_value kvp = bb_get_key_value(line);
        if (strcmp(kvp.key, "VGL_DISPLAY")) {

        } else if (strcmp(kvp.key, "STOP_SERVICE_ON_EXIT")) {

        } else if (strcmp(kvp.key, "X_CONFFILE")) {

        } else if (strcmp(kvp.key, "VGL_COMPRESS")) {

        } else if (strcmp(kvp.key, "ECO_MODE")) {

        } else if (strcmp(kvp.key, "FALLBACK_START")) {

        }
      }
    }
  }
  fclose(cf);
  return 0;
}

/**
 *  Print a little note on usage
 */
static void print_usage(int exit_val) {
  // Print help message and exit with exit code
  printf("%s version %s\n\n", bb_status.program_name, GITVERSION);
  printf("Usage: %s [options] [--] [application to run] [application options]\n", bb_status.program_name);
  printf("  Options:\n");
  //client-only options
  printf("      -m [METHOD]\tConnection method to use for VirtualGL.\n");
  //server-only options
  printf("      -d\tRun as daemon.\n");
  printf("      -x [PATH]\txorg.conf file to use.\n");
  //common options
  printf("      -q\tBe quiet (sets verbosity to zero)\n");
  printf("      -v\tBe more verbose (can be used multiple times)\n");
  printf("      -X #\tX display number to use.\n");
  printf("      -l [PATH]\tLD driver path to use.\n");
  printf("      -u [PATH]\tUnix socket to use.\n");
  printf("      -h\tShow this help screen.\n");
  printf("\n");
  exit(exit_val);
}

/// Read the commandline parameters
static void read_cmdline_config( int argc, char ** argv ){
  /* Parse the options, set flags as necessary */
  int c;
  while ((c = getopt(argc, argv, "+dqvx:g:X:u:l:m:c:h|help")) != -1) {
    switch (c) {
      case 'h'://help
        print_usage(EXIT_SUCCESS);
        break;
      case 'd'://daemonize
        bb_status.runmode = BB_RUN_DAEMON;
        break;
      case 'q'://quiet mode
        bb_status.verbosity = VERB_NONE;
        break;
      case 'v'://increase verbosity level by one
        bb_status.verbosity++;
        break;
      case 'x'://xorg.conf path
        snprintf(bb_config.x_conf_file, BUFFER_SIZE, "%s", optarg);
        break;
      case 'X'://X display number
        snprintf(bb_config.x_display, BUFFER_SIZE, "%s", optarg);
        break;
      case 'u'://Unix socket to use
        snprintf(bb_config.socket_path, BUFFER_SIZE, "%s", optarg);
        break;
      case 'g'://group name to use
        snprintf(bb_config.gid_name, BUFFER_SIZE, "%s", optarg);
        break;
      case 'l'://LD driver path
        snprintf(bb_config.ld_path, BUFFER_SIZE, "%s", optarg);
        break;
      case 'm'://vglclient method
        snprintf(bb_config.vgl_compress, BUFFER_SIZE, "%s", optarg);
        break;
      case 'c'://config file
        snprintf(bb_config.bb_conf_file, BUFFER_SIZE, "%s", optarg);
        break;
      default:
        // Unrecognized option
        print_usage(EXIT_FAILURE);
        break;
    }
  }
  
}

/// Read commandline parameters and config file.
/// Works by first setting compiled-in defaults,
/// then parsing commandline parameters,
/// then loading the config file,
/// finally again parsing commandline parameters.
void init_config( int argc, char ** argv ){
  /* set status */
  int i = 0;
  int lastslash = 0;
  //find the last slash in the program path
  while (argv[0][i] != 0){
    if (argv[0][i] == '/'){lastslash = i;}
    ++i;
  }
  //set program name
  snprintf(bb_status.program_name, BUFFER_SIZE, "%s", argv[0]+lastslash+1);
  bb_status.verbosity = VERB_WARN;
  bb_status.bb_socket = -1;
  bb_status.appcount = 0;
  bb_status.errors[0] = 0;//set first byte to NULL = empty string
  bb_status.x_pid = 0;
  if (strcmp(bb_status.program_name, "optirun") == 0){
    bb_status.runmode = BB_RUN_APP;
  }else{
    bb_status.runmode = BB_RUN_SERVER;
  }

  /* standard configuration */
  snprintf(bb_config.x_display, BUFFER_SIZE, CONF_XDISP);
  snprintf(bb_config.x_conf_file, BUFFER_SIZE, CONF_XORG);
  snprintf(bb_config.bb_conf_file, BUFFER_SIZE, CONFIG_FILE);
  snprintf(bb_config.ld_path, BUFFER_SIZE, CONF_LDPATH);
  snprintf(bb_config.socket_path, BUFFER_SIZE, CONF_SOCKPATH);
  snprintf(bb_config.gid_name, BUFFER_SIZE, CONF_GID);
  bb_config.pm_enabled = CONF_PMENABLE;
  bb_config.stop_on_exit = CONF_STOPONEXIT;
  snprintf(bb_config.vgl_compress, BUFFER_SIZE, CONF_VGLCOMPRESS);
  

  // parse commandline configuration (for config file, if changed)
  read_cmdline_config(argc, argv);
  // parse config file
  read_configuration();
  // parse commandline configuration again (so config file params are overwritten)
  read_cmdline_config(argc, argv);
}
