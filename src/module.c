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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "module.h"
#include "bblogger.h"
#include "bbrun.h"

/**
 * Checks in /proc/modules whether a kernel module is loaded
 *
 * @param driver The name of the driver (not a filename)
 * @return 1 if the module is loaded, 0 otherwise
 */
int module_is_loaded(char *driver) {
  // use the same buffer length as lsmod
  char buffer[4096];
  FILE * bbs = fopen("/proc/modules", "r");
  int ret = 0;
  /* assume mod_len <= sizeof(buffer) */
  int mod_len = strlen(driver);

  if (bbs == 0) {//error opening, return -1
    bb_log(LOG_DEBUG, "Couldn't open /proc/modules");
    return -1;
  }
  while (fgets(buffer, sizeof(buffer), bbs)) {
    /* match "module" with "module " and not "module-blah" */
    if (!strncmp(buffer, driver, mod_len) && isspace(buffer[mod_len])) {
      /* module is found */
      ret = 1;
      break;
    }
  }
  fclose(bbs);
  return ret;
}

/**
 * Attempts to load a module. If the module has not been loaded after ten
 * seconds, give up
 *
 * @param module_name The filename of the module to be loaded
 * @param driver The name of the driver to be loaded
 * @return 1 if the driver is succesfully loaded, 0 otherwise
 */
int module_load(char *module_name, char *driver) {
  if (module_is_loaded(driver) == 0) {
    /* the module has not loaded yet, try to load it */
    bb_log(LOG_INFO, "Loading driver %s (module %s)\n", driver, module_name);
    char *mod_argv[] = {
      "modprobe",
      module_name,
      NULL
    };
    bb_run_fork_wait(mod_argv, 10);
    if (module_is_loaded(driver) == 0) {
      bb_log(LOG_ERR, "Module %s could not be loaded (timeout?)\n", module_name);
      return 0;
    }
  }
  return 1;
}

/**
 * Attempts to unload a module if loaded, for ten seconds before
 * giving up
 *
 * @param driver The name of the driver (not a filename)
 * @return 1 if the driver is succesfully unloaded, 0 otherwise
 */
int module_unload(char *driver) {
  if (module_is_loaded(driver) == 1) {
    bb_log(LOG_INFO, "Unloading %s driver\n", driver);
    char *mod_argv[] = {
      "rmmod",
      driver,
      NULL
    };
    bb_run_fork_wait(mod_argv, 10);
    if (module_is_loaded(driver) == 1) {
      bb_log(LOG_ERR, "Unloading %s driver timed out.\n", driver);
      return 0;
    }
  }
  return 1;
}

/**
 * Checks whether a kernel module is available for loading
 *
 * @param module_name The module name to be checked (filename or alias)
 * @return 1 if the module is available for loading, 0 otherwise
 */
int module_is_available(char *module_name) {
  /* HACK to support call from optirun */
  char *modprobe_bin = "/sbin/modprobe";
  if (access(modprobe_bin, X_OK)) {
    /* if /sbin/modprobe is not found, pray that PATH contains it */
    modprobe_bin = "modprobe";
  }
  char *mod_argv[] = {
    modprobe_bin,
    "--dry-run",
    "--quiet",
    module_name,
    NULL
  };
  return bb_run_fork(mod_argv, 1) == EXIT_SUCCESS;
}
