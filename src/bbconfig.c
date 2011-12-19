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
#include <getopt.h>

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
    bb_log(LOG_ERR, "Error parsing configuration file: %s\n", strerror(err_val));
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
  bb_log(LOG_DEBUG, "Reading configuration file: %s\n",bb_config.bb_conf_file);
  FILE *cf = fopen(bb_config.bb_conf_file, "r");
  if (cf == 0) { /* An error ocurred */
    bb_log(LOG_ERR, "Error in config file: %s\n", strerror(errno));
    bb_log(LOG_INFO, "Using default configuration");
    return 1;
  }
  char line[BUFFER_SIZE];
  while (fgets(line, sizeof line, cf) != NULL) {
    stripws(line);
    /* Ignore empty lines and comments */
    if ((line[0] != '#') && (line[0] != '\n')) {
      /* Parse configuration based on the run mode */
      struct bb_key_value kvp = bb_get_key_value(line);
      if (strcmp(kvp.key, "VGL_DISPLAY") == 0) {
        snprintf(bb_config.x_display, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: x_display = %s\n", bb_config.x_display);
      } else if (strcmp(kvp.key, "STOP_SERVICE_ON_EXIT") == 0) {
        bb_config.stop_on_exit = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: stop_on_exit = %d\n", bb_config.stop_on_exit);
      } else if (strcmp(kvp.key, "X_CONFFILE") == 0) {
        snprintf(bb_config.x_conf_file, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: x_conf_file = %s\n", bb_config.x_conf_file);
      } else if (strcmp(kvp.key, "VGL_COMPRESS") == 0) {
        snprintf(bb_config.vgl_compress, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: vgl_compress = %s\n", bb_config.vgl_compress);
      } else if (strcmp(kvp.key, "ECO_MODE") == 0) {
      } else if (strcmp(kvp.key, "FALLBACK_START") == 0) {
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
  int opt = 0;
  optind = 0;
  static const char *optString = "+Dqvx:d:s:g:l:c:C:Vh?";
  static const struct option longOpts[] = {
    {"daemon",0,0,'D'},
    {"quiet",0,0,'q'},
    {"silent",0,0,'q'},
    {"verbose",0,0,'v'},
    {"xconf",1,0,'x'},
    {"display",1,0,'d'},
    {"socket",1,0,'s'},
    {"group",1,0,'g'},
    {"ldpath",1,0,'l'},
    {"vgl-compress",1,0,'c'},
    {"config",1,0,'C'},
    {"help",1,0,'h'},
    {"silent",0,0,'q'},
    {"version",0,0,'V'},
    {0, 0, 0, 0}
  };
  while ((opt = getopt_long(argc, argv, optString, longOpts, 0)) != -1){
    switch (opt){
      case 'D'://daemonize
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
      case 'd'://X display number
        snprintf(bb_config.x_display, BUFFER_SIZE, "%s", optarg);
        break;
      case 's'://Unix socket to use
        snprintf(bb_config.socket_path, BUFFER_SIZE, "%s", optarg);
        break;
      case 'g'://group name to use
        snprintf(bb_config.gid_name, BUFFER_SIZE, "%s", optarg);
        break;
      case 'l'://LD driver path
        snprintf(bb_config.ld_path, BUFFER_SIZE, "%s", optarg);
        break;
      case 'c'://vglclient method
        snprintf(bb_config.vgl_compress, BUFFER_SIZE, "%s", optarg);
        break;
      case 'C'://config file
        snprintf(bb_config.bb_conf_file, BUFFER_SIZE, "%s", optarg);
        break;
      case 'V'://print version
        printf("Version: %s\n", GITVERSION);
        exit(EXIT_SUCCESS);
        break;
      default:
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
    if (argv[0][i] == '/'){lastslash = i+1;}
    ++i;
  }
  //set program name
  snprintf(bb_status.program_name, BUFFER_SIZE, "%s", argv[0]+lastslash);
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
