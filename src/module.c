/*
 * Copyright (c) 2011-2013, The Bumblebee Project
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
#include <libkmod.h>
#include <errno.h>
#include "module.h"
#include "bblogger.h"
#include "bbrun.h"
#include "bbconfig.h"

int module_unload_recursive(struct kmod_module *mod);

/**
 * Checks whether a kernel module is loaded
 *
 * @param driver The name of the driver (not a filename)
 * @return 1 if the module is loaded, 0 otherwise
 */
int module_is_loaded(char *driver) {
  int err, state;
  struct kmod_module *mod;

  err = kmod_module_new_from_name(bb_status.kmod_ctx, driver, &mod);
  if(err < 0) {
    bb_log(LOG_DEBUG, "kmod_module_new_from_name(%s) failed.\n", driver);
    return -1;
  }

  state = kmod_module_get_initstate(mod);
  kmod_module_unref(mod);

  return state == KMOD_MODULE_LIVE;
}

/**
 * Attempts to load a module.
 *
 * @param module_name The filename of the module to be loaded
 * @param driver The name of the driver to be loaded
 * @return 1 if the driver is succesfully loaded, 0 otherwise
 */
int module_load(char *module_name, char *driver) {
  int err = 0;
  int flags = KMOD_PROBE_IGNORE_LOADED;
  struct kmod_list *l, *list = NULL;

  if (module_is_loaded(driver) == 0) {
    /* the module has not loaded yet, try to load it */

    bb_log(LOG_INFO, "Loading driver '%s' (module '%s')\n", driver, module_name);
    err = kmod_module_new_from_lookup(bb_status.kmod_ctx, module_name, &list);

    if(err < 0) {
      bb_log(LOG_DEBUG, "kmod_module_new_from_lookup(%s) failed (err: %d).\n",
        module_name, err);
      return 0;
    }

    if(list == NULL) {
      bb_log(LOG_ERR, "Module '%s' not found.\n");
      return 0;
    }

    kmod_list_foreach(l, list) {
      struct kmod_module *mod = kmod_module_get_module(l);

      bb_log(LOG_DEBUG, "Loading module '%s'.\n", kmod_module_get_name(mod));
      err = kmod_module_probe_insert_module(mod, flags, NULL, NULL, NULL, 0);

      if (err < 0) {
        bb_log(LOG_DEBUG, "kmod_module_probe_insert_module(%s) failed (err: %d).\n",
          kmod_module_get_name(mod), err);
      }

      kmod_module_unref(mod);

      if(err < 0) {
        break;
      }
    }

    kmod_module_unref_list(list);
  }

  return err >= 0;
}

/**
 * Unloads module and modules that are depending on this module.
 *
 * @param mod Reference to libkmod module
 * @return 1 if the module is succesfully unloaded, 0 otherwise
 */
int module_unload_recursive(struct kmod_module *mod) {
  int err = 0, flags = 0, refcnt;
  struct kmod_list *holders;

  holders = kmod_module_get_holders(mod);
  if (holders != NULL) {
    struct kmod_list *itr;

    kmod_list_foreach(itr, holders) {
      struct kmod_module *hm = kmod_module_get_module(itr);
      err = module_unload_recursive(hm);
      kmod_module_unref(hm);

      if(err < 0) {
        break;
      }
    }
    kmod_module_unref_list(holders);
  }

  refcnt = kmod_module_get_refcnt(mod);
  if(refcnt == 0) {
    bb_log(LOG_INFO, "Unloading module %s\n", kmod_module_get_name(mod));
    err = kmod_module_remove_module(mod, flags);
  } else {
    bb_log(LOG_ERR, "Failed to unload module '%s' (ref count: %d).\n",
      kmod_module_get_name(mod), refcnt);
    err = 1;
  }

  return err == 0;
}

/**
 * Attempts to unload a module if loaded.
 *
 * @param driver The name of the driver (not a filename)
 * @return 1 if the driver is succesfully unloaded, 0 otherwise
 */
int module_unload(char *driver) {
  int err;
  struct kmod_module *mod;
  if (module_is_loaded(driver) == 1) {
    err = kmod_module_new_from_name(bb_status.kmod_ctx, driver, &mod);

    if(err < 0) {
      bb_log(LOG_DEBUG, "kmod_module_new_from_name(%s) failed (err: %d).\n",
        driver, err);
      return 0;
    }

    err = module_unload_recursive(mod);
    kmod_module_unref(mod);

    return err;
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
  int err, available;
  struct kmod_list *list = NULL;

  err = kmod_module_new_from_lookup(bb_status.kmod_ctx, module_name, &list);

  if(err < 0) {
    bb_log(LOG_DEBUG, "kmod_module_new_from_lookup(%s) failed (err: %d).\n",
      module_name, err);
  }

  available = (err == 0) && list != NULL;

  kmod_module_unref_list(list);

  return available;
}
