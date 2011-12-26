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
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>

struct bb_status_struct bb_status;
struct bb_config_struct bb_config;

struct bb_key_value {
  char key[BUFFER_SIZE];
  char value[BUFFER_SIZE];
};

#define CONFIG_PARSE_ERR(err_msg, line) \
  bb_log(LOG_ERR, "Error parsing configuration: %s. line: %s\n", err_msg, line)

static size_t strip_lead_trail_ws(char *dest, char *str, size_t len);

/**
 * Takes a line and breaks it into a key-value pair
 *
 * @param line String to be broken into a key-value pair
 * @param kvpair A pointer to a key/value struct to store data in
 * @return 0 success, non-zero on failure
 */
static int bb_get_key_value(const char *line, struct bb_key_value *kvpair) {
  char *equals_pos = strstr(line, "=");
  if (!equals_pos) {
    CONFIG_PARSE_ERR("expected an =-sign.", line);
    return 1;
  }

  // the length of the key and value including null byte must be smaller than
  // BUFFER_SIZE
  int key_len = equals_pos - line;
  if (key_len >= BUFFER_SIZE) {
    CONFIG_PARSE_ERR("key name too long", line);
    return 1;
  }
  int val_len = strlen(line) - key_len - strlen("=");
  if (val_len >= BUFFER_SIZE) {
    CONFIG_PARSE_ERR("value too long", line);
    return 1;
  }

  // consider only the leading part of the line for key
  memcpy(kvpair->key, line, key_len);
  kvpair->key[key_len] = 0;
  strip_lead_trail_ws(kvpair->key, kvpair->key, BUFFER_SIZE);
  // the remainder can directly be trimmed for value
  strip_lead_trail_ws(kvpair->value,
      (char*) line + key_len + strlen("="), BUFFER_SIZE);

  // remove the single or double quotes around a value
  char value_first_char = *kvpair->value;
  val_len = strlen(kvpair->value);
  if (val_len >= 2 && (value_first_char == '\'' || value_first_char == '"')) {
    if (kvpair->value[val_len - 1] == value_first_char) {
      // bye last quote
      kvpair->value[val_len - 1] = 0;
      // hello value without quote
      memmove(kvpair->value, kvpair->value + 1, val_len + 1);
    }
    // XXX perhaps log than an incorrect value was found like "val?
  }
  return 0;
}

/**
 * Strips leading and trailing whitespaces from a string. The original string
 * is modified, trailing spaces are removed
 *
 * @param dest String in which the trimmed result is to be stored
 * @param str String to be cleared of leading and trailing whitespaces
 * @param len Maximum number of bytes to be copied
 * @return The length of the trimmed string. This may be larger than len if the
 * buffer is too small
 */
static size_t strip_lead_trail_ws(char *dest, char *str, size_t len) {
  char *end;
  /* the length of the trimmed string */
  size_t actual_len;

  // Remove leading spaces
  while (isspace(*str)) {
    str++;
  }
  // all whitespace
  if (len == 0 || *str == 0) {
    *dest = 0;
    return 0;
  }
  // Remove trailing spaces
  end = str + strlen(str) - 1;
  while ((end > str) && (isspace(*end))) {
    end--;
  }
  actual_len = end - str + 1;

  // if the string is smaller. cast to hide compiler notice, len is always > 0
  len = actual_len < len ? actual_len : (unsigned) len - 1;
  memmove(dest, str, len);
  // Add null terminator to end
  dest[len] = 0;

  return actual_len;
}

/**
 *  Read the configuration file.
 *
 *  @return 0 on success.
 */
static int read_configuration(void) {
  bb_log(LOG_DEBUG, "Reading configuration file: %s\n", bb_config.bb_conf_file);
  FILE *cf = fopen(bb_config.bb_conf_file, "r");
  if (cf == 0) { /* An error ocurred */
    bb_log(LOG_ERR, "Error in config file: %s\n", strerror(errno));
    bb_log(LOG_INFO, "Using default configuration\n");
    return 1;
  }
  char line[BUFFER_SIZE];
  while (fgets(line, sizeof line, cf) != NULL) {
    strip_lead_trail_ws(line, line, BUFFER_SIZE);
    /* Ignore empty lines and comments */
    if ((line[0] != '#') && (line[0] != '\n')) {
      /* Parse configuration based on the run mode */
      struct bb_key_value kvp;
      /* skip lines that could not be parsed */
      if (bb_get_key_value(line, &kvp))
        continue;

      if (strncmp(kvp.key, "VGL_DISPLAY", 11) == 0) {
        snprintf(bb_config.x_display, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: x_display = %s\n", bb_config.x_display);
      } else if (strncmp(kvp.key, "STOP_SERVICE_ON_EXIT", 20) == 0) {
        bb_config.stop_on_exit = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: stop_on_exit = %d\n", bb_config.stop_on_exit);
      } else if (strncmp(kvp.key, "X_CONFFILE", 10) == 0) {
        snprintf(bb_config.x_conf_file, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: x_conf_file = %s\n", bb_config.x_conf_file);
      } else if (strncmp(kvp.key, "VGL_COMPRESS", 12) == 0) {
        snprintf(bb_config.vgl_compress, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: vgl_compress = %s\n", bb_config.vgl_compress);
      } else if (strncmp(kvp.key, "ENABLE_POWER_MANAGEMENT", 23) == 0) {
        bb_config.pm_enabled = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: pm_enabled = %d\n", bb_config.pm_enabled);
      } else if (strncmp(kvp.key, "FALLBACK_START", 15) == 0) {
        bb_config.fallback_start = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: fallback_start = %d\n", bb_config.fallback_start);
      } else if (strncmp(kvp.key, "BUMBLEBEE_GROUP", 16) == 0) {
        snprintf(bb_config.gid_name, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: gid_name = %s\n", bb_config.gid_name);
      } else if (strncmp(kvp.key, "DRIVER", 7) == 0) {
        snprintf(bb_config.driver, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: driver = %s\n", bb_config.driver);
      } else if (strncmp(kvp.key, "NV_LIBRARY_PATH", 16) == 0) {
        snprintf(bb_config.ld_path, BUFFER_SIZE, "%s", kvp.value);
        bb_log(LOG_DEBUG, "value set: ld_path = %s\n", bb_config.ld_path);
      } else if (strncmp(kvp.key, "CARD_SHUTDOWN_STATE", 20) == 0) {
        bb_config.card_shutdown_state = atoi(kvp.value);
        bb_log(LOG_DEBUG, "value set: card_shutdown_state = %d\n", bb_config.card_shutdown_state);
      }
    }
  }
  fclose(cf);
  return 0;
}

/// Prints a single line, for use by print_usage, with alignment.
static void print_usage_line(char * opt, char * desc) {
  printf("  %-25s%s\n", opt, desc);
}

/**
 *  Print a little note on usage
 */
static void print_usage(int exit_val) {
  // Print help message and exit with exit code
  printf("%s version %s\n\n", bb_status.program_name, GITVERSION);
  if (strncmp(bb_status.program_name, "optirun", 8) == 0) {
    printf("Usage: %s [options] [--] [application to run] [application options]\n", bb_status.program_name);
  } else {
    printf("Usage: %s [options]\n", bb_status.program_name);
  }
  printf(" Options:\n");
  if (strncmp(bb_status.program_name, "optirun", 8) == 0) {
    //client-only options
    print_usage_line("-c [METHOD]", "Connection method to use for VirtualGL.");
    print_usage_line("--vgl-compress [METHOD]", "Connection method to use for VirtualGL.");
  } else {
    //server-only options
    print_usage_line("-D", "Run as daemon.");
    print_usage_line("--daemon", "Run as daemon.");
    print_usage_line("-x [PATH]", "xorg.conf file to use.");
    print_usage_line("--xconf [PATH]", "xorg.conf file to use.");
    print_usage_line("-g [GROUPNAME]", "Name of group to change to.");
    print_usage_line("--group [GROUPNAME]", "Name of group to change to.");
  }
  //common options
  print_usage_line("-q", "Be quiet (sets verbosity to zero)");
  print_usage_line("--quiet", "Be quiet (sets verbosity to zero)");
  print_usage_line("--silent", "Be quiet (sets verbosity to zero)");
  print_usage_line("-v", "Be more verbose (can be used multiple times)");
  print_usage_line("--verbose", "Be more verbose (can be used multiple times)");
  print_usage_line("-d [DISPLAY NAME]", "X display number to use.");
  print_usage_line("--display [DISPLAY NAME]", "X display number to use.");
  print_usage_line("-C [PATH]", "Configuration file to use.");
  print_usage_line("--config [PATH]", "Configuration file to use.");
  print_usage_line("-l [PATH]", "LD driver path to use.");
  print_usage_line("--ldpath [PATH]", "LD driver path to use.");
  print_usage_line("-s [PATH]", "Unix socket to use.");
  print_usage_line("--socket [PATH]", "Unix socket to use.");
  print_usage_line("-h", "Show this help screen.");
  print_usage_line("--help", "Show this help screen.");
  printf("\n");
  exit(exit_val);
}

/// Read the commandline parameters
static void read_cmdline_config(int argc, char ** argv) {
  /* Parse the options, set flags as necessary */
  int opt = 0;
  optind = 0;
  static const char *optString = "+Dqvx:d:s:g:l:c:C:Vh?";
  static const struct option longOpts[] = {
    {"daemon", 0, 0, 'D'},
    {"quiet", 0, 0, 'q'},
    {"silent", 0, 0, 'q'},
    {"verbose", 0, 0, 'v'},
    {"xconf", 1, 0, 'x'},
    {"display", 1, 0, 'd'},
    {"socket", 1, 0, 's'},
    {"group", 1, 0, 'g'},
    {"ldpath", 1, 0, 'l'},
    {"vgl-compress", 1, 0, 'c'},
    {"config", 1, 0, 'C'},
    {"help", 1, 0, 'h'},
    {"silent", 0, 0, 'q'},
    {"version", 0, 0, 'V'},
    {0, 0, 0, 0}
  };
  while ((opt = getopt_long(argc, argv, optString, longOpts, 0)) != -1) {
    switch (opt) {
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
void init_config(int argc, char ** argv) {
  //set program name
  strncpy(bb_status.program_name, basename(argv[0]), BUFFER_SIZE);
  bb_status.verbosity = VERB_WARN;
  bb_status.bb_socket = -1;
  bb_status.appcount = 0;
  bb_status.errors[0] = 0; //set first byte to NULL = empty string
  bb_status.x_pid = 0;
  if (strcmp(bb_status.program_name, "optirun") == 0) {
    bb_status.runmode = BB_RUN_APP;
  } else {
    bb_status.runmode = BB_RUN_SERVER;
  }

  /* standard configuration */
  strncpy(bb_config.x_display, CONF_XDISP, BUFFER_SIZE);
  strncpy(bb_config.bb_conf_file, CONFIG_FILE, BUFFER_SIZE);
  strncpy(bb_config.ld_path, CONF_LDPATH, BUFFER_SIZE);
  strncpy(bb_config.socket_path, CONF_SOCKPATH, BUFFER_SIZE);
  strncpy(bb_config.gid_name, CONF_GID, BUFFER_SIZE);
  strncpy(bb_config.x_conf_file, CONF_XORG, BUFFER_SIZE);
  bb_config.pm_enabled = CONF_PMENABLE;
  bb_config.stop_on_exit = CONF_STOPONEXIT;
  bb_config.fallback_start = CONF_FALLBACKSTART;
  bb_config.card_shutdown_state = CONF_SHUTDOWNSTATE;
  strncpy(bb_config.vgl_compress, CONF_VGLCOMPRESS, BUFFER_SIZE);
  // default to auto-detect
  strncpy(bb_config.driver, "", BUFFER_SIZE);

  // parse commandline configuration (for config file, if changed)
  read_cmdline_config(argc, argv);
  // parse config file
  read_configuration();
  // parse commandline configuration again (so config file params are overwritten)
  read_cmdline_config(argc, argv);

  //check xorg.conf for %s, replace it by driver name
  char tmpstr[BUFFER_SIZE] = {0};
  // position after last %s
  char *pos = bb_config.x_conf_file;
  char *next;
  while ((next = strstr(pos, "DRIVER")) != 0 && strlen(tmpstr) < BUFFER_SIZE) {
    unsigned int len = next - pos;
    if (len + strlen(tmpstr) >= BUFFER_SIZE) {
      // don't overflow the buffer and keep room for the null byte
      len = BUFFER_SIZE - strlen(tmpstr) - 1;
    }
    strncat(tmpstr, pos, len);
    if (strlen(tmpstr) < BUFFER_SIZE)
      strncat(tmpstr, bb_config.driver, BUFFER_SIZE - strlen(tmpstr) - 1);

    // the next search starts at the position after %s
    pos = next + strlen("DRIVER");
  }
  // append the remainder after the last %s if any and overwrite the setting
  strncat(tmpstr, pos, BUFFER_SIZE - strlen(tmpstr) - 1);
  strncpy(bb_config.x_conf_file, tmpstr, BUFFER_SIZE);
}

/**
 * Prints the current configuration with verbosity level LOG_DEBUG
 */
void config_dump(void) {
  //print configuration as debug messages
  bb_log(LOG_DEBUG, "Active configuration:\n");
  bb_log(LOG_DEBUG, " X display: %s\n", bb_config.x_display);
  bb_log(LOG_DEBUG, " xorg.conf file: %s\n", bb_config.x_conf_file);
  bb_log(LOG_DEBUG, " bumblebeed config file: %s\n", bb_config.bb_conf_file);
  bb_log(LOG_DEBUG, " LD_LIBRARY_PATH: %s\n", bb_config.ld_path);
  bb_log(LOG_DEBUG, " Socket path: %s\n", bb_config.socket_path);
  bb_log(LOG_DEBUG, " GID name: %s\n", bb_config.gid_name);
  bb_log(LOG_DEBUG, " Power management: %i\n", bb_config.pm_enabled);
  bb_log(LOG_DEBUG, " Stop X on exit: %i\n", bb_config.stop_on_exit);
  bb_log(LOG_DEBUG, " VGL Compression: %s\n", bb_config.vgl_compress);
  bb_log(LOG_DEBUG, " Driver: %s\n", bb_config.driver);
  bb_log(LOG_DEBUG, " Card shutdown state: %i\n", bb_config.card_shutdown_state);
}
