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

#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include "bbconfig.h"
#include "bblogger.h"
#include "module.h"

/* config values for PM methods, edit bb_pm_method in bbconfig.h as well! */
const char *bb_pm_method_string[PM_METHODS_COUNT] = {
    "none",
    "auto",
     /* the below names are used in switch/switching.c */
    "bbswitch",
    "switcheroo",
};

struct bb_status_struct bb_status;
struct bb_config_struct bb_config;

struct bb_key_value {
  char key[BUFFER_SIZE];
  char value[BUFFER_SIZE];
};

/**
 * Little function to log parsing errors
 * @Deprecated
 */
static inline void bb_config_parse_err(const char* msg, const char* line) {
  bb_log(LOG_ERR, "Error parsing configuration: %s. line: %s\n", msg, line);
}

static size_t strip_lead_trail_ws(char *dest, char *str, size_t len);

/**
 * Takes a pointer to a char pointer, resizing and copying the string value to it.
 * @Deprecated
 */
void set_string_value(char ** configstring, char * newvalue) {
  //free the string if it already existed.
  if (*configstring != 0) {
    free(*configstring);
    *configstring = 0;
  }
  //malloc a new buffer of strlen, plus one for the terminating null byte
  *configstring = malloc(strlen(newvalue)+1);
  if (*configstring != 0) {
    //copy the string if successful
    strcpy(*configstring, newvalue);
  } else {
    //something, somewhere, went terribly wrong
    bb_log(LOG_ERR, "Could not allocate %i bytes for new config value, setting to empty string!\n", strlen(newvalue)+1);
    *configstring = malloc(1);
    if (*configstring == 0) {
      bb_log(LOG_ERR, "FATAL: Could not allocate even 1 byte for config value!\n");
      bb_log(LOG_ERR, "Aborting - cannot continue without stable config!\n");
      exit(1);
    }
    (*configstring)[0] = 0;
  }
}//set_string_value

/**
 * Determines the boolean value for a given string
 * @param value A value to be analyzed
 * @return 1 if the value resolves to a truth value, 0 otherwise
 * @Deprecated
 */
int boolean_value(char *val) {
  /* treat void, an empty string, N, n and zero as false */
  if (!val || !*val || *val == 'N' || *val == 'n' || *val == '0') {
    return 0;
  }
  return 1;
}

/**
 * Takes a line and breaks it into a key-value pair
 *
 * @param line String to be broken into a key-value pair
 * @param kvpair A pointer to a key/value struct to store data in
 * @return 0 success, non-zero on failure
 * @Deprecated
 */
static int bb_get_key_value(const char *line, struct bb_key_value *kvpair) {
  char *equals_pos = strstr(line, "=");
  if (!equals_pos) {
    bb_config_parse_err("expected an =-sign.", line);
    return 1;
  }

  // the length of the key and value including null byte must be smaller than
  // BUFFER_SIZE
  int key_len = equals_pos - line;
  if (key_len >= BUFFER_SIZE) {
    bb_config_parse_err("key name too long", line);
    return 1;
  }
  int val_len = strlen(line) - key_len - strlen("=");
  if (val_len >= BUFFER_SIZE) {
    bb_config_parse_err("value too long", line);
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
 * @Deprecated
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
 * Converts a string to the internal representation of a PM method
 * @param value The string to be converted
 * @return An index in the PM methods array
 */
static enum bb_pm_method bb_pm_method_from_string(char *value) {
  /* loop backwards through all possible values. If no valid value is found,
   * assume the first element ("none") */
  enum bb_pm_method method_index = PM_METHODS_COUNT;
  while (method_index-- > 0) {
    if (strcmp(value, bb_pm_method_string[method_index]) == 0) {
      break;
    }
  }
  return method_index;
}

/**
 *  Read the configuration file.
 *
 *  @return 0 on success.
 *  @Deprecated
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
    if ((line[0] != '#') && (line[0] != '\0')) {
      /* Parse configuration based on the run mode */
      struct bb_key_value kvp;
      /* skip lines that could not be parsed */
      if (bb_get_key_value(line, &kvp))
        continue;

      if (strcmp(kvp.key, "VGL_DISPLAY") == 0) {
        set_string_value(&bb_config.x_display, kvp.value);
        bb_log(LOG_DEBUG, "value set: x_display = %s\n", bb_config.x_display);
      } else if (strcmp(kvp.key, "STOP_SERVICE_ON_EXIT") == 0) {
        bb_config.stop_on_exit = boolean_value(kvp.value);
        bb_log(LOG_DEBUG, "value set: stop_on_exit = %d\n", bb_config.stop_on_exit);
      } else if (strcmp(kvp.key, "X_CONFFILE") == 0) {
        set_string_value(&bb_config.x_conf_file, kvp.value);
        bb_log(LOG_DEBUG, "value set: x_conf_file = %s\n", bb_config.x_conf_file);
      } else if (strcmp(kvp.key, "VGL_COMPRESS") == 0) {
        set_string_value(&bb_config.vgl_compress, kvp.value);
        bb_log(LOG_DEBUG, "value set: vgl_compress = %s\n", bb_config.vgl_compress);
      } else if (strcmp(kvp.key, "PM_METHOD") == 0) {
        enum bb_pm_method method_index = bb_pm_method_from_string(kvp.value);
        bb_config.pm_method = method_index;
        bb_log(LOG_DEBUG, "value set: pm_method = %s\n",
                bb_pm_method_string[method_index]);
      } else if (strcmp(kvp.key, "FALLBACK_START") == 0) {
        bb_config.fallback_start = boolean_value(kvp.value);
        bb_log(LOG_DEBUG, "value set: fallback_start = %d\n", bb_config.fallback_start);
      } else if (strcmp(kvp.key, "BUMBLEBEE_GROUP") == 0) {
        set_string_value(&bb_config.gid_name, kvp.value);
        bb_log(LOG_DEBUG, "value set: gid_name = %s\n", bb_config.gid_name);
      } else if (strcmp(kvp.key, "DRIVER") == 0) {
        set_string_value(&bb_config.driver, kvp.value);
        bb_log(LOG_DEBUG, "value set: driver = %s\n", bb_config.driver);
      } else if (strcmp(kvp.key, "DRIVER_MODULE") == 0) {
        set_string_value(&bb_config.module_name, kvp.value);
        bb_log(LOG_DEBUG, "value set: module_name = %s\n", bb_config.module_name);
      } else if (strcmp(kvp.key, "NV_LIBRARY_PATH") == 0) {
        set_string_value(&bb_config.ld_path, kvp.value);
        bb_log(LOG_DEBUG, "value set: ld_path = %s\n", bb_config.ld_path);
      } else if (strcmp(kvp.key, "MODULE_PATH") == 0) {
        set_string_value(&bb_config.mod_path, kvp.value);
        bb_log(LOG_DEBUG, "value set: mod_path = %s\n", bb_config.mod_path);
      } else if (strcmp(kvp.key, "CARD_SHUTDOWN_STATE") == 0) {
        bb_config.card_shutdown_state = boolean_value(kvp.value);
        bb_log(LOG_DEBUG, "value set: card_shutdown_state = %d\n", bb_config.card_shutdown_state);
      }
    }
  }
  fclose(cf);
  return 0;
}

/// Prints a single line, for use by print_usage, with alignment.
static void print_usage_line(char * opt, char * desc) {
  printf("  %-35s%s\n", opt, desc);
}

/**
 * Prints a usage message and exits with given exit code
 * @param exit_val The exit code to be passed to exit()
 */
void print_usage(int exit_val) {
  int is_optirun = bb_status.runmode == BB_RUN_APP ||
    bb_status.runmode == BB_RUN_STATUS;
  printf("%s version %s\n\n", bb_status.program_name, GITVERSION);
  if (is_optirun) {
    printf("Usage: %s [options] [--] [application to run] [application options]\n", bb_status.program_name);
  } else {
    printf("Usage: %s [options]\n", bb_status.program_name);
  }
  printf(" Options:\n");
  if (is_optirun) {
    //client-only options
    print_usage_line("--vgl-compress / -c [METHOD]", "Connection method to use for VirtualGL.");
    print_usage_line("--failsafe={Y|N}", "If Y, the program even starts if the"
            " server is unavailable");
  } else {
    //server-only options
    print_usage_line("--daemon / -D", "Run as daemon.");
    print_usage_line("--xconf / -x [PATH]", "xorg.conf file to use.");
    print_usage_line("--group / -g [GROUPNAME]", "Name of group to change to.");
    print_usage_line("--driver [nvidia / nouveau]", "Force use of a certain GPU driver.");
    print_usage_line("--module-path / -m [PATH]", "ModulePath to use for xorg (nvidia-only).");
    print_usage_line("--driver-module / -k [NAME]", "Name of kernel module to be"
            " loaded if different from the driver");
    print_usage_line("--pidfile", "File in which the PID is written");
  }
  //common options
  print_usage_line("--quiet / --silent / -q", "Be quiet (sets verbosity to zero)");
  print_usage_line("--verbose / -v", "Be more verbose (can be used multiple times)");
  print_usage_line("--display / -d [DISPLAY NAME]", "X display number to use.");
  print_usage_line("--config / -C [PATH]", "Configuration file to use.");
  print_usage_line("--ldpath / -l [PATH]", "LD driver path to use (nvidia-only).");
  print_usage_line("--socket / -s [PATH]", "Unix socket to use.");
  print_usage_line("--help / -h", "Show this help screen.");
  printf("\n");
  exit(exit_val);
}

/**
 * Parses common (shared) command line options
 * @param opt The short option
 * @param value Value for the option if any
 * @return 1 if the option has been processed, 0 otherwise
 */
static int bbconfig_parse_common(int opt, char *value) {
  switch (opt) {
    case 'q'://quiet mode
      bb_status.verbosity = VERB_NONE;
      break;
    case 'v'://increase verbosity level by one
      if (bb_status.verbosity < VERB_ALL) {
        bb_status.verbosity++;
      }
      break;
    case 'd'://X display number
      set_string_value(&bb_config.x_display, value);
      break;
    case 's'://Unix socket to use
      set_string_value(&bb_config.socket_path, value);
      break;
    case 'l'://LD driver path
      set_string_value(&bb_config.ld_path, value);
      break;
    case 'V'://print version
      printf("Version: %s\n", GITVERSION);
      exit(EXIT_SUCCESS);
      break;
    default:
      /* no options parsed */
      return 0;
  }
  return 1;
}

/**
 * Parses commandline options
 * @param argc The arguments count
 * @param argv The arguments values
 * @param config_only 1 if the config file is the only option to be parsed
 */
static void bbconfig_parse_opts(int argc, char *argv[], int config_only) {
  /* Parse the options, set flags as necessary */
  int opt;
  optind = 0;
  const char *optString = bbconfig_get_optstr();
  const struct option *longOpts = bbconfig_get_lopts();
  while ((opt = getopt_long(argc, argv, optString, longOpts, 0)) != -1) {
    if (opt == '?') {
      /* if an option was not recognized */
      print_usage(EXIT_FAILURE);
    } if (opt == 'C') {
      set_string_value(&bb_config.bb_conf_file, optarg);
    } else if (!config_only) {
      /* try to find local options first, then try common options */
      if (bbconfig_parse_options(opt, optarg) ||
              bbconfig_parse_common(opt, optarg)) {
        /* option has been parsed, continue with the next options */
        continue;
      }
      /* XXX is it possible to print the usage message earlier? */
      print_usage(EXIT_FAILURE);
    }
  }
}

/**
 * Parse configuration file given by bb_config.bb_conf_file
 *
 * @return 0 on success
 */
static int bbconfig_parse_conf(void) {
    //Old behavior
    //return read_configuration();

    bb_log(LOG_DEBUG, "Reading file: %s\n", bb_config.bb_conf_file);
    GKeyFile *bbcfg;
    GKeyFileFlags flags = G_KEY_FILE_NONE;
    GError *err = NULL;

    bbcfg = g_key_file_new();
    if(!g_key_file_load_from_file(bbcfg, bb_config.bb_conf_file, flags, &err)) {
        bb_log(LOG_WARNING, "Could not open configuration file: %s\n", bb_config.bb_conf_file);
        bb_log(LOG_WARNING, "Using default configuration\n");
        return 1;
    }

    // First check for a key existence then parse it with appropriate format
    // Use false as default for boolean arguments.
    // TODO:optirun/bumblebeed must be parsed according to RUN_MODE

    char* section;
    char* key;
    // Client settings
    // [optirun]
    section = "optirun";
    key = "VGLTransport";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.vgl_compress = g_key_file_get_string(bbcfg, section, key, &err);
    }
    key = "AllowFallbackToIGC";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.fallback_start = g_key_file_get_boolean(bbcfg, section, key, &err);
    }

    // Server settings
    // [bumblebeed]
    section = "bumblebeed";
    key = "VirtualDisplay";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.x_display = g_key_file_get_string(bbcfg, section, key, &err);
    }
    key = "KeepUnusedXServer";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.stop_on_exit = !g_key_file_get_boolean(bbcfg, section, key, &err);
    }
    key = "Driver";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.driver = g_key_file_get_string(bbcfg, section, key, &err);
    }
    key = "ServerGroup";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.gid_name = g_key_file_get_string(bbcfg, section, key, &err);
    }
    key = "TurnCardOffAtExit";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.card_shutdown_state = !g_key_file_get_boolean(bbcfg, section, key, &err);
    }

    // Driver settings
    // [nouveua] or [nvidia]
    section = bb_config.driver;
    key = "KernelDriver";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.module_name = g_key_file_get_string(bbcfg, section, key, &err);
    }
    key = "LibraryPath";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.ld_path = g_key_file_get_string(bbcfg, section, key, &err);
    }
    key = "XorgModulePath";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.mod_path = g_key_file_get_string(bbcfg, section, key, &err);
    }
    key = "PMMethod";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
         enum bb_pm_method pm_method_index = bb_pm_method_from_string(g_key_file_get_string(bbcfg, section, key, &err));
         bb_config.pm_method = pm_method_index;
    }
    key = "XorgConfFile";
    if (NULL != g_key_file_get_value(bbcfg, section, key, &err)) {
        bb_config.x_conf_file = g_key_file_get_string(bbcfg, section, key, &err);
    }
}

/**
 * Set options that must be set before opening logs or loading configuration
 * @param argc Arguments count
 * @param argv Argument values
 * @param runmode The running mode of the program
 */
void init_early_config(int argc, char **argv, int runmode) {
  /* clear existing configuration and reset pointers */
  memset(&bb_status, 0, sizeof bb_status);
  /* set program name */
  set_string_value(&bb_status.program_name, basename(argv[0]));
  set_string_value(&bb_status.errors, "");//we start without errors, yay!
  bb_status.verbosity = VERB_WARN;
  bb_status.bb_socket = -1;
  bb_status.appcount = 0;
  bb_status.x_pid = 0;
  bb_status.runmode = runmode;
}

/**
 * Parse configuration file and command line arguments
 * @param argc Arguments count
 * @param argv Argument values
 */
void init_config(int argc, char **argv) {
  /* clear pointers and settings */
  memset(&bb_config, 0, sizeof bb_config);
  /* set defaults if not set already */
  set_string_value(&bb_config.x_display, CONF_XDISP);
  set_string_value(&bb_config.bb_conf_file, CONFIG_FILE);
  set_string_value(&bb_config.ld_path, CONF_LDPATH);
  set_string_value(&bb_config.mod_path, CONF_MODPATH);
  set_string_value(&bb_config.socket_path, CONF_SOCKPATH);
  set_string_value(&bb_config.gid_name, CONF_GID);
  set_string_value(&bb_config.x_conf_file, CONF_XORG);
  set_string_value(&bb_config.vgl_compress, CONF_VGLCOMPRESS);
  // default to auto-detect
  set_string_value(&bb_config.driver, "");
  set_string_value(&bb_config.module_name, CONF_DRIVER_MODULE);
  bb_config.pm_method = bb_pm_method_from_string(CONF_PM_METHOD);
  bb_config.stop_on_exit = CONF_STOPONEXIT;
  bb_config.fallback_start = CONF_FALLBACKSTART;
  bb_config.card_shutdown_state = CONF_SHUTDOWNSTATE;
  set_string_value(&bb_config.pid_file, CONF_PID_FILE);

  /* parse commandline configuration (for config file, if any) */
  bbconfig_parse_opts(argc, argv, 1);

  /* parse config file */
  //read_configuration();
  bbconfig_parse_conf();

  /* parse remaining command line options */
  bbconfig_parse_opts(argc, argv, 0);
}

/**
 * Prints the current configuration with verbosity level LOG_DEBUG
 */
void config_dump(void) {
  //print configuration as debug messages
  bb_log(LOG_DEBUG, "Active configuration:\n");
  /* common options */
  bb_log(LOG_DEBUG, " bumblebeed config file: %s\n", bb_config.bb_conf_file);
  bb_log(LOG_DEBUG, " X display: %s\n", bb_config.x_display);
  bb_log(LOG_DEBUG, " LD_LIBRARY_PATH: %s\n", bb_config.ld_path);
  bb_log(LOG_DEBUG, " Socket path: %s\n", bb_config.socket_path);
  if (bb_status.runmode == BB_RUN_SERVER || bb_status.runmode == BB_RUN_DAEMON) {
    /* daemon options */
    bb_log(LOG_DEBUG, " pidfile: %s\n", bb_config.pid_file);
    bb_log(LOG_DEBUG, " xorg.conf file: %s\n", bb_config.x_conf_file);
    bb_log(LOG_DEBUG, " ModulePath: %s\n", bb_config.mod_path);
    bb_log(LOG_DEBUG, " GID name: %s\n", bb_config.gid_name);
    bb_log(LOG_DEBUG, " Power method: %s\n",
      bb_pm_method_string[bb_config.pm_method]);
    bb_log(LOG_DEBUG, " Stop X on exit: %i\n", bb_config.stop_on_exit);
    bb_log(LOG_DEBUG, " Driver: %s\n", bb_config.driver);
    bb_log(LOG_DEBUG, " Driver module: %s\n", bb_config.module_name);
    bb_log(LOG_DEBUG, " Card shutdown state: %i\n",
            bb_config.card_shutdown_state);
  } else {
    /* client options */
    bb_log(LOG_DEBUG, " VGL Compression: %s\n", bb_config.vgl_compress);
  }
}

/**
 * Checks the configuration for errors and report them
 *
 * @return 0 if no errors are detected, non-zero otherwise
 */
int config_validate(void) {
  int error = 0;
  if (*bb_config.driver) {
    char *mod = bb_config.driver;
    if (!module_is_available(mod)) {
      error = 1;
      bb_log(LOG_ERR, "Driver '%s' is not found.\n", mod);
    }
  } else {
    bb_log(LOG_ERR, "Invalid configuration: no driver configured.\n");
    error = 1;
  }
  if (!error) {
    bb_log(LOG_DEBUG, "Configuration test passed.\n");
  }
  return error;
}

/**
 * Sets error messages if any problems occur.
 * Resets stored error when called with argument 0.
 */
void set_bb_error(char * msg) {
  if (msg && msg[0] != 0) {
    set_string_value(&bb_status.errors, msg);
    bb_log(LOG_ERR, "%s\n", msg);
  } else {
    //clear set error message, if any.
    if (bb_status.errors[0] != 0) {
      set_string_value(&bb_status.errors, "");
    }
  }
}//set_bb_error
