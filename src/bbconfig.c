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
 * bbconfig.c: Bumblebee configuration file handler
 */

#include "bbconfig.h"
#include "bblogger.h"
#include "config.h"
#include <errno.h>
#include <ctype.h>
#include <assert.h>

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
int read_configuration() {
  /* standard configuration */
  snprintf(bb_config.x_display, BUFFER_SIZE, CONF_XDISP);
  snprintf(bb_config.x_conf_file, BUFFER_SIZE, CONF_XORG);
  snprintf(bb_config.ld_path, BUFFER_SIZE, CONF_XORG);
  snprintf(bb_config.socket_path, BUFFER_SIZE, CONF_XORG);
  bb_config.pm_enabled = CONF_PMENABLE;
  bb_config.stop_on_exit = CONF_STOPONEXIT;

  FILE *cf = fopen(CONFIG_FILE, "r");
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
