/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Peter Lekensteyn <lekensteyn@gmail.com>
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

#include <string.h>
#include "bbconfig.h"
#include "module.h"
#include "bblogger.h"
#include "driver.h"

/**
 * Check what drivers are available and autodetect if possible. Driver, module
 * library path and module path are set
 */
void driver_detect(void) {
  /* determine driver to be used */
  if (*bb_config.driver) {
    bb_log(LOG_DEBUG, "Skipping auto-detection, using configured driver"
            " '%s'\n", bb_config.driver);
  } else if (strlen(CONF_DRIVER)) {
    /* if the default driver is set, use that */
    set_string_value(&bb_config.driver, CONF_DRIVER);
    bb_log(LOG_DEBUG, "Using compile default driver '%s'", CONF_DRIVER);
  } else if (module_is_loaded("nouveau")) {
    /* loaded drivers take precedence over ones available for modprobing */
    set_string_value(&bb_config.driver, "nouveau");
    set_string_value(&bb_config.module_name, "nouveau");
    bb_log(LOG_DEBUG, "Detected nouveau driver\n");
  } else if (module_is_available(CONF_DRIVER_MODULE_NVIDIA)) {
    /* Ubuntu and Mandriva use nvidia-current.ko. nvidia cannot be compiled into
     * the kernel, so module_is_available makes module_is_loaded redundant */
    set_string_value(&bb_config.driver, "nvidia");
    set_string_value(&bb_config.module_name, CONF_DRIVER_MODULE_NVIDIA);
    bb_log(LOG_DEBUG, "Detected nvidia driver (module %s)\n",
            CONF_DRIVER_MODULE_NVIDIA);
  } else if (module_is_available("nouveau")) {
    set_string_value(&bb_config.driver, "nouveau");
    set_string_value(&bb_config.module_name, "nouveau");
    bb_log(LOG_DEBUG, "Detected nouveau driver\n");
  }

  if (!*bb_config.module_name) {
    /* no module has been configured, set a sensible one based on driver */
    if (strcmp(bb_config.driver, "nvidia") == 0 &&
            module_is_available(CONF_DRIVER_MODULE_NVIDIA)) {
      set_string_value(&bb_config.module_name, CONF_DRIVER_MODULE_NVIDIA);
    } else {
      set_string_value(&bb_config.module_name, bb_config.driver);
    }
  }

  if (strcmp(bb_config.driver, "nvidia") == 0) {
    set_string_value(&bb_config.ld_path, CONF_LDPATH_NVIDIA);
    set_string_value(&bb_config.mod_path, CONF_MODPATH_NVIDIA);
  }
}
