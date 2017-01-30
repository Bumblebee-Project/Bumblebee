/*
 * Copyright (c) 2011-2013, The Bumblebee Project
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
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
  "nouveau",
};

struct bb_status_struct bb_status;
struct bb_config_struct bb_config;

/**
 * Returns a gboolean from true/false strings
 * @return TRUE if str="true", FALSE otherwise
 */
gboolean bb_bool_from_string(char* str) {
  if (strcmp(str, "true") == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
 * Sets a value for configstring and free() existing values
 * @param configstring A pointer to the destination
 * @param newvalue A string to be set
 */
void free_and_set_value(char **configstring, char *newvalue) {
  if (*configstring != NULL) {
    free(*configstring);
  }
  *configstring = newvalue;
}

/**
 * Takes a pointer to a char pointer, resizing and copying the string value to it.
 */
void set_string_value(char ** configstring, char * newvalue) {
  //free the string if it already existed.
  if (*configstring != 0) {
    free(*configstring);
    *configstring = 0;
  }
  //malloc a new buffer of strlen, plus one for the terminating null byte
  *configstring = malloc(strlen(newvalue) + 1);
  if (*configstring != 0) {
    //copy the string if successful
    strcpy(*configstring, newvalue);
  } else {
    //something, somewhere, went terribly wrong
    bb_log(LOG_ERR, "Could not allocate %i bytes for new config value, setting to empty string!\n", strlen(newvalue) + 1);
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
 * Converts a string to the internal representation of a PM method
 * @param value The string to be converted
 * @return An index in the PM methods array
 */
enum bb_pm_method bb_pm_method_from_string(char *value) {
  /* loop backwards through all possible values. If no valid value is found,
   * assume the first element ("none") */
  enum bb_pm_method method_index = PM_METHODS_COUNT;
  while (method_index > 0) {
    if (strcmp(value, bb_pm_method_string[--method_index]) == 0) {
      break;
    }
  }
  return method_index;
}

/**
 * Prints a usage message and exits with given exit code
 * @param exit_val The exit code to be passed to exit(). If non-zero, an hint is
 * printed to use --help. Otherwise a help message is printed
 */
void print_usage(int exit_val) {
  FILE *out = stdout;
  int is_optirun = bb_status.runmode == BB_RUN_APP ||
          bb_status.runmode == BB_RUN_STATUS;

  if (exit_val != EXIT_SUCCESS) {
    fprintf(stderr, "Try `%s --help' for more information.\n",
            bb_status.program_name);
    exit(exit_val);
  }

  if (is_optirun) {
    printf("Usage: %s [OPTION]... command [command options...]\n",
            bb_status.program_name);
    fputs("Run an application using the discrete video card.\n", out);
  } else {
    printf("Usage: %s [OPTION]...\n", bb_status.program_name);
    fputs("Daemon for controlling the discrete nVidia video card on Optimus"
            " systems.\n", out);
  }
  printf("\n");
  if (is_optirun) {
    //client-only options
    fputs("\
      --failsafe      run a program even if the nvidia card is unavailable\n\
      --no-failsafe   do not run a program if the nvidia card is unavailable\n\
      --no-xorg       do not start secondary X server (implies -b none)\n\
  -b, --bridge METHOD  acceleration/displaying bridge to use. Valid values\n\
                       are auto, virtualgl and primus. The --vgl-* options\n\
                       only make sense when using the virtualgl bridge,\n\
                       while the --primus-* options apply only when using\n\
                       the primus bridge.\n\
		       Additionally, value none is recognized, and its effect\n\
		       is to add paths to driver libraries to LD_LIBRARY_PATH\n\
		       (useful for nvidia-settings and CUDA applications)\n\
  -c, --vgl-compress METHOD  image compression or transport to use with \n\
                               VirtualGL. Valid values for METHOD are proxy,\n\
                               jpeg, rgb, xv and yuv. Changing this setting\n\
                               may affect performance, CPU usage and image\n\
                               quality\n\
      --vgl-options OPTS   a space-separated list of command options to be\n\
                             passed to vglrun. Useful for debugging virtualgl\n\
                             by passing options to it like +tr. These OPTS\n\
                             override the settings from optirun so be careful\n\
                             with setting it\n\
      --primus-ldpath PATH  a colon-separated list of paths which are searched\n\
                            for the primus libGL.so.1\n",
            out);
  } else {
    //server-only options
    fputs("\
  -D, --daemon          run daemonized (backgrounded). Implies --use-syslog\n\
  -x, --xconf FILE      xorg.conf file to use\n\
      --xconfdir DIR    xorg.conf.d directory to use\n\
  -g, --group GROUP     allow GROUP to communicate with the daemon\n\
      --driver DRIVER   the driver to use for the nvidia card. Valid values\n\
                          are nouveau and nvidia. This option also effects\n\
                          the driver section that will be used from the\n\
                          configuration file\n\
  -m, --module-path PATH  ModulePath to use for Xorg (only useful for nvidia)\n\
  -k, --driver-module NAME    Name of kernel module to be loaded if different\n\
                                from the driver\n\
      --pm-method METHOD  method to use for disabling the discrete video card,\n\
                            valid values are auto, bbswitch, nouveau,\n\
                            switcheroo and none. auto selects a sensible method,\n\
                            bbswitch (kernel module) is available for nvidia\n\
                            and nouveau drivers, nouveau uses this driver's PM\n\
                            faculities, switcheroo (vga_switcheroo) is usually for\n\
                            nouveau and radeon drivers and none disables PM\n\
                            completely\n",
            out);
#ifdef WITH_PIDFILE
    fputs("\
      --pidfile FILE    file in which the process ID is written. An empty\n\
                          value disables cretion of a pidfile. Note that\n\
                          the file must not already exist\n\
      --use-syslog      redirect all messages to syslog\n", out);
#endif
  }
  /* common options */
  fputs("\
  -q, --quiet, --silent   suppresses all logging messages\n\
  -v, --verbose           increase the verbosity level of log messages. It\n\
                            can be specified up to two times (or five if\n\
                            --quiet is used)\n\
      --debug             show all logging messages by setting the verbosity\n\
                            level to the maximum\n\
  -C, --config FILE       retrieve settings for Bumblebee from FILE\n", out);
  if (is_optirun) {
    fputs("\
  -d, --display VDISPLAY  find the Bumblebee X server on VDISPLAY. Do not\n\
                            confuse this option with the DISPLAY environment\n\
                            variable. By default, PATH is queried from the\n\
                            daemon\n\
  -l, --ldpath PATH       libraries like libGL.so are searched in PATH\n\
                            (useful for the nvidia driver). By default, PATH\n\
                            is queried from the\n", out);
  } else {
    fputs("\
  -d, --display VDISPLAY  start the Bumblebee X server on VDISPLAY. Do not\n\
                            confuse this option with the DISPLAY environment\n\
                            variable\n\
  -l, --ldpath PATH       libraries like nvidia_drv.so are searched in PATH\n\
                            (useful for the nvidia driver)\n", out);
  }
  fputs("\
  -s, --socket FILENAME   use FILENAME for communication with the daemon\n\
  -h, --help              display this help and exit\n\
  --version               output version information and exit\n", out);
  if (is_optirun) {
    fputs("\n\
Examples:\n\
  optirun glxspheres      Runs the graphics demo supplied with Virtual GL.\n\
  optirun -c yuv glxspheres  Runs a program with the yuv transport method for\n\
                             better performance.\n\
  optirun firefox http://example.com    Run a program with arguments.\n", out);
  }
  fputs("\n\
Report bugs on <http://Bumblebee-Project.org/issues>\n\
Bumblebee homepage: <http://Bumblebee-Project.org/>\n", out);
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
    case 'd'://X display number
      set_string_value(&bb_config.x_display, value);
      break;
    case 'l'://LD driver path
      set_string_value(&bb_config.ld_path, value);
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
void bbconfig_parse_opts(int argc, char *argv[], int conf_round) {
  /* Parse the options, set flags as necessary */
  int opt;
  optind = 0;
  const char *optString = bbconfig_get_optstr();
  const struct option *longOpts = bbconfig_get_lopts();
  while ((opt = getopt_long(argc, argv, optString, longOpts, 0)) != -1) {
    if (opt == '?') {
      /* if an option was not recognized */
      print_usage(EXIT_FAILURE);
    }
    if (conf_round == PARSE_STAGE_LOG && bb_status.runmode == BB_RUN_SERVER) {
      /* hack to get logging ready before parsing other options */
      switch (opt) {
        case 'D':
        case OPT_USE_SYSLOG:
          bb_status.use_syslog = TRUE;
          break;
      }
    } else if (conf_round == PARSE_STAGE_PRECONF) {
      int is_optirun = bb_status.runmode == BB_RUN_APP ||
              bb_status.runmode == BB_RUN_STATUS;
      switch (opt) {
        case 'C':
          set_string_value(&bb_config.bb_conf_file, optarg);
          break;
        case 'v':
          if (bb_status.verbosity < VERB_ALL) {
            bb_status.verbosity++;
          }
          break;
        case 'q'://quiet mode
          bb_status.verbosity = VERB_NONE;
          break;
        case OPT_DEBUG://debug mode
          bb_status.verbosity = VERB_ALL;
          break;
        case 's': /* Unix socket to use for communication */
          set_string_value(&bb_config.socket_path, optarg);
          break;
        case 'V'://print version
          printf("%s (Bumblebee) %s\n",
                  is_optirun ? "optirun" : "bumblebeed", GITVERSION);
          printf("Copyright (C) 2011 The Bumblebee Project\n");
          printf("License GPLv3+: GNU GPL version 3 or later"
                  " <http://gnu.org/licenses/gpl.html>.\n");
          printf("This is free software: you are free to change and redistribute"
                  " it.\n");
          printf("There is NO WARRANTY, to the extent permitted by law.\n");
          exit(EXIT_SUCCESS);
          break;
        case 'h':
          print_usage(EXIT_SUCCESS);
          break;
      }
    } else if (conf_round == PARSE_STAGE_DRIVER) {
      switch (opt) {
        case OPT_DRIVER:
          set_string_value(&bb_config.driver, optarg);
          break;
      }
    } else if (conf_round == PARSE_STAGE_OTHER) {
      /* try to find local options first, then try common options */
      if (bbconfig_parse_options(opt, optarg) ||
              bbconfig_parse_common(opt, optarg)) {
        /* option has been parsed, continue with the next options */
        continue;
      }
    }
  }
}

/**
 * Parse configuration file given by bb_config.bb_conf_file
 *
 * @return A pointer to an GKeyFile object or NULL on failure
 */
GKeyFile *bbconfig_parse_conf(void) {
  //Old behavior
  //return read_configuration();

  bb_log(LOG_DEBUG, "Reading file: %s\n", bb_config.bb_conf_file);
  GKeyFile *bbcfg;
  GKeyFileFlags flags = G_KEY_FILE_NONE;
  GError *err = NULL;

  bbcfg = g_key_file_new();
  if (!g_key_file_load_from_file(bbcfg, bb_config.bb_conf_file, flags, &err)) {
    bb_log(LOG_WARNING, "Could not open configuration file: %s\n", bb_config.bb_conf_file);
    bb_log(LOG_WARNING, "Using default configuration\n");
    g_error_free(err);
    g_key_file_free(bbcfg);
    return NULL;
  }

  // First check for a key existence then parse it with appropriate format
  // Use false as default for boolean arguments.
  // TODO:optirun/bumblebeed must be parsed according to RUN_MODE

  char* section;
  char* key;
  // Client settings
  // [optirun]
  section = "optirun";
  key = "Bridge";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.optirun_bridge, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  key = "PrimusLibraryPath";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.primus_ld_path, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  key = "VGLTransport";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.vgl_compress, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  key = "AllowFallbackToIGC";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    bb_config.fallback_start = g_key_file_get_boolean(bbcfg, section, key, NULL);
  }

  // Server settings
  // [bumblebeed]
  section = "bumblebeed";
  key = "VirtualDisplay";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.x_display, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  key = "KeepUnusedXServer";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    bb_config.stop_on_exit = !g_key_file_get_boolean(bbcfg, section, key, NULL);
  }
  key = "Driver";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    char *driver = g_key_file_get_string(bbcfg, section, key, NULL);
    /* empty driv */
    if (*driver != 0) {
      free_and_set_value(&bb_config.driver, driver);
      bb_log(LOG_INFO, "Configured driver: %s\n", bb_config.driver);
    } else {
      g_free(driver);
    }
  }
  key = "ServerGroup";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.gid_name, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  key = "TurnCardOffAtExit";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    bb_config.card_shutdown_state = !g_key_file_get_boolean(bbcfg, section, key, NULL);
  }
  key = "XorgConfDir";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.x_conf_dir, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  key = "XorgBinary";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.xorg_binary, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  return bbcfg;
}

/**
 * Loads driver settings from an open GKeyFile
 * @param bbcfg A pointer to a GKeyFile
 * @param driver The string containing the driver to be loaded
 */
void bbconfig_parse_conf_driver(GKeyFile *bbcfg, char *driver) {
  GError *err = NULL;
  char *key, *section;

  section = malloc(strlen("driver-") + strlen(driver) + 1);
  if (section == NULL) {
    /* why can't we just assume that there is always enough memory available?
     * If there is no memory, the program cannot do anything useful anyway. */
    bb_log(LOG_WARNING, "Driver settings could not be loaded: out of memory\n");
    return;
  }
  strcpy(section, "driver-");
  strcat(section, driver);

  key = "KernelDriver";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    char *module_name = g_key_file_get_string(bbcfg, section, key, NULL);
    /* if KernelDriver is empty, the default behavior is to copy Driver which
     * is done in driver_detect() */
    if (*module_name) {
      free_and_set_value(&bb_config.module_name, module_name);
    } else {
      g_free(module_name);
    }
  }
  key = "LibraryPath";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.ld_path, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  key = "XorgModulePath";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.mod_path, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  key = "PMMethod";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    char *val = g_key_file_get_string(bbcfg, section, key, NULL);
    enum bb_pm_method pm_method_index = bb_pm_method_from_string(val);
    bb_config.pm_method = pm_method_index;
    g_free(val);
  }
  key = "XorgConfFile";
  if (g_key_file_has_key(bbcfg, section, key, NULL)) {
    free_and_set_value(&bb_config.x_conf_file, g_key_file_get_string(bbcfg, section, key, NULL));
  }
  if (err != NULL) {
    g_error_free(err);
  }
  free(section);
}

/**
 * Set options that must be set before opening logs or loading configuration
 * @param argv Argument values
 * @param runmode The running mode of the program
 */
void init_early_config(char **argv, int runmode) {
  /* clear existing configuration and reset pointers */
  memset(&bb_status, 0, sizeof bb_status);
  set_string_value(&bb_status.errors, ""); //we start without errors, yay!
  bb_status.verbosity = VERB_NOTICE;
  bb_status.bb_socket = -1;
  bb_status.appcount = 0;
  bb_status.x_pid = 0;
  bb_status.x_pipe[0] = -1;
  bb_status.x_pipe[1] = -1;
  bb_status.runmode = runmode;
  bb_status.program_name = argv[0];
}

/**
 * Parse configuration file and command line arguments
 */
void init_config(void) {
  /* clear pointers and settings */
  memset(&bb_config, 0, sizeof bb_config);
  /* set defaults if not set already */
  set_string_value(&bb_config.x_display, CONF_XDISP);
  set_string_value(&bb_config.bb_conf_file, CONFIG_FILE);
  set_string_value(&bb_config.ld_path, "");
  set_string_value(&bb_config.mod_path, "");
  set_string_value(&bb_config.socket_path, CONF_SOCKPATH);
  set_string_value(&bb_config.gid_name, CONF_GID);
  set_string_value(&bb_config.x_conf_file, CONF_XORG);
  set_string_value(&bb_config.x_conf_dir, CONF_XORG_DIR);
  set_string_value(&bb_config.xorg_binary, CONF_XORG_BINARY);
  set_string_value(&bb_config.optirun_bridge, CONF_BRIDGE);
  set_string_value(&bb_config.primus_ld_path, CONF_PRIMUS_LD_PATH);
  set_string_value(&bb_config.vgl_compress, CONF_VGLCOMPRESS);
  // default to auto-detect
  set_string_value(&bb_config.driver, "");
  set_string_value(&bb_config.module_name, "");
  bb_config.pm_method = bb_pm_method_from_string(CONF_PM_METHOD);
  bb_config.stop_on_exit = bb_bool_from_string(CONF_KEEPONEXIT);
  bb_config.fallback_start = bb_bool_from_string(CONF_FALLBACKSTART);
  bb_config.card_shutdown_state = bb_bool_from_string(CONF_TURNOFFATEXIT);
#ifdef WITH_PIDFILE
  set_string_value(&bb_config.pid_file, CONF_PIDFILE);
#endif
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
#ifdef WITH_PIDFILE
    bb_log(LOG_DEBUG, " pidfile: %s\n", bb_config.pid_file);
#endif
    bb_log(LOG_DEBUG, " xorg.conf file: %s\n", bb_config.x_conf_file);
    bb_log(LOG_DEBUG, " xorg.conf.d dir: %s\n", bb_config.x_conf_dir);
    bb_log(LOG_DEBUG, " Xorg binary: %s\n", bb_config.xorg_binary);
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
    bb_log(LOG_DEBUG, " Accel/display bridge: %s\n", bb_config.optirun_bridge);
    bb_log(LOG_DEBUG, " VGL Compression: %s\n", bb_config.vgl_compress);
    bb_log(LOG_DEBUG, " VGLrun extra options: %s\n", bb_config.vglrun_options ? bb_config.vglrun_options : "");
    bb_log(LOG_DEBUG, " Primus LD Path: %s\n", bb_config.primus_ld_path);
  }
}

/**
 * Checks the configuration for errors and report them
 *
 * @return 0 if no errors are detected, non-zero otherwise
 */
int config_validate(void) {
  int error = 0;
  if (*bb_config.module_name) {
    char *mod = bb_config.module_name;
    if (!module_is_available(mod)) {
      error = 1;
      bb_log(LOG_ERR, "Module '%s' is not found.\n", mod);
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
    //only store error if no error stored yet
    //earliest error is the most important!
    if (bb_status.errors[0] == 0){
      set_string_value(&bb_status.errors, msg);
    }
    bb_log(LOG_ERR, "%s\n", msg);
  } else {
    //clear set error message, if any.
    if (bb_status.errors[0] != 0) {
      set_string_value(&bb_status.errors, "");
    }
  }
}//set_bb_error
