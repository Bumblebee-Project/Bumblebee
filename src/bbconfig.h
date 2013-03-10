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
 * bbconfig.h: Bumblebee configuration file handler
 */

#include <unistd.h> //for pid_t
#include <limits.h> //for CHAR_MAX
#include <glib.h>

/* Daemon states */
#define BB_DAEMON 1
#define BB_NODEAMON 0

/* Parsing rounds */
enum {
    PARSE_STAGE_LOG,
    PARSE_STAGE_PRECONF,
    PARSE_STAGE_DRIVER,
    PARSE_STAGE_OTHER,
};

/* common command line params */
#define BBCONFIG_COMMON_OPTSTR "+qvd:s:l:C:hV"
#define BBCONFIG_COMMON_LOPTS \
    {"quiet", 0, 0, 'q'},\
    {"silent", 0, 0, 'q'},\
    {"verbose", 0, 0, 'v'},\
    {"display", 1, 0, 'd'},\
    {"socket", 1, 0, 's'},\
    {"ldpath", 1, 0, 'l'},\
    {"config", 1, 0, 'C'},\
    {"help", 0, 0, 'h'},\
    {"version", 0, 0, 'V'},\
    {"debug", 0, 0, OPT_DEBUG},\
    {0, 0, 0, 0}

const char *bbconfig_get_optstr(void);
const struct option *bbconfig_get_lopts(void);
int bbconfig_parse_options(int opt, char *value);

/* use a value that cannot be a valid char for getopt */
enum {
    OPT_DRIVER = CHAR_MAX + 1,
    OPT_FAILSAFE,
    OPT_NO_FAILSAFE,
    OPT_VGL_OPTIONS,
    OPT_STATUS,
    OPT_PIDFILE,
    OPT_USE_SYSLOG,
    OPT_DEBUG,
    OPT_PM_METHOD,
};

/* Verbosity levels */
enum verbosity_level {
    VERB_NONE,
    VERB_ERR,
    VERB_WARN,
    VERB_NOTICE,
    VERB_INFO,
    VERB_DEBUG,
    VERB_ALL
};

/* Running modes */
enum bb_run_mode {
    BB_RUN_SERVER = 0,
    BB_RUN_DAEMON = 1,
    BB_RUN_APP = 2,
    BB_RUN_STATUS = 4,
    BB_RUN_EXIT = 99
};

/* Power management methods, edit bb_pm_method_string in bbconfig.c as well! */
enum bb_pm_method {
    PM_DISABLED,
    PM_AUTO, /* at detection time, this value will be changed */
    PM_BBSWITCH,
    PM_VGASWITCHEROO,
    PM_METHODS_COUNT /* not a method but a marker for the end */
};
const char *bb_pm_method_string[PM_METHODS_COUNT];

/* String buffer size */
#define BUFFER_SIZE 1024

/* Structure containing the status of the application */
struct bb_status_struct {
    enum verbosity_level verbosity; ///Verbosity level of messages.
    int bb_socket; /// The socket file descriptor of the application.
    unsigned int appcount; /// Count applications using the X server.
    char * errors; /// Error message if any. First byte is 0 otherwise.
    enum bb_run_mode runmode; /// Running mode.
    pid_t x_pid;
    int x_pipe[2];//pipes for reading/writing output from X's stdout/stderr
    gboolean use_syslog;
    char *program_name;
};

/* Structure containing the configuration. */
struct bb_config_struct {
    char * x_display; /// X display number to use.
    char * x_conf_file; /// Path to the X configuration file.
    char * bb_conf_file; /// Path to the bumblebeed configuration file.
    char * ld_path; /// LD_LIBRARY_PATH to launch applications.
    char * mod_path; /// ModulePath for xorg.
    char * socket_path; /// Path to the server socket.
    char * gid_name; /// Group name for setgid.
    enum bb_pm_method pm_method; /// Which method to use for power management.
    int stop_on_exit; /// Whether to stop the X server on last optirun instance exit.
    int fallback_start; /// Wheter the application should be launched on the integrated card when X is not available.
    char * optirun_bridge; /// Accel/display bridge for optirun.
    char * vgl_compress; /// VGL transport method.
    char *vglrun_options; /* extra options passed to vglrun */
    char * driver; /// Driver to use (nvidia or nouveau).
    char * module_name; /* Kernel module to be loaded for the driver.
                                    * If empty, driver will be used. This is
                                    * for Ubuntu which uses nvidia-current */
    int card_shutdown_state;
#ifdef WITH_PIDFILE
    char *pid_file; /* pid file for storing the daemons PID */
#endif
};

extern struct bb_status_struct bb_status;
extern struct bb_config_struct bb_config;

/* Early initialization of bb_status */
void init_early_config(char **argv, int runmode);
/* Parse configuration from command line and configuration files */
void init_config(void);
void config_dump(void);
int config_validate(void);

/**
 * Sets error messages if any problems occur.
 * Resets stored error when called with argument 0.
 */
void set_bb_error(char * msg);

void free_and_set_value(char **configstring, char *newvalue);

/**
 * Takes a pointer to a char pointer, resizing and copying the string value to it.
 */
void set_string_value(char ** configstring, char * newvalue);

struct option *config_get_longopts(struct option *longopts, size_t items);

void print_usage(int exit_val);

void bbconfig_parse_opts(int argc, char *argv[], int conf_round);

GKeyFile *bbconfig_parse_conf(void);
void bbconfig_parse_conf_driver(GKeyFile *bbcfg, char *driver);

gboolean bb_bool_from_string(char* str);

enum bb_pm_method bb_pm_method_from_string(char *value);

size_t ensureZeroTerminated(char *buff, size_t size, size_t max);
