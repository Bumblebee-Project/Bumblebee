/*
 * Copyright (c) 2011-2013, The Bumblebee Project
 * Author: Jaron Viëtor AKA "Thulinma" <jaron@vietors.com>
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

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/file.h>
#include "../bblogger.h"
#include "switching.h"
#include "../module.h"

static int nouveau_fd = -1;

/**
 * Reports the status of nouveau
 *
 * @return SWITCH_OFF if nouveau is loaded (so card is power-managed),
 * SWITCH_ON if card is on.
 */
enum switch_state nouveau_status(void) {
  return nouveau_fd != -1 ? SWITCH_OFF : SWITCH_ON;
}//nouveau_status

/**
 * Whether nouveau is available for use
 *
 * @param info A struct containing information which would help with the
 * decision whether nouveau is usable or not
 * @return 1 if available for use for PM, 0 otherwise
 */
int nouveau_is_available(struct switch_info info) {
  if (strcmp(info.configured_pm, "nouveau") != 0) {
    bb_log(LOG_INFO, "Skipping nouveau PM method because it is not"
            " explicitly selected in the configuration.\n");
    return 0;
  }
  return module_is_available("nouveau");
}

/**
 * Unloads nouveau, leaving card unmanaged.
 */
void nouveau_on(void) {
  /* Close nouveau device */
  if (nouveau_fd != -1) {
    close(nouveau_fd);
    nouveau_fd = -1;
  }

  if (!module_unload("nouveau")) {
    bb_log(LOG_ERR, "couldn't unload nouveau\n");
  } else {
    bb_log(LOG_DEBUG, "nouveau successfully unloaded\n");
  }
}//nouveau_on

static const open_retry_delay_us = 5000;

/**
 * Loads nouveau, hopefully powering card down.
 */
void nouveau_off(void) {
  DIR *dp;
  struct dirent *ep;
  int card_no = -1;
  char buf[PATH_MAX];
  int open_retries = 10;

  if (nouveau_fd != -1) {
    return;
  }

  if (!module_load("nouveau", "nouveau", "runpm=1 modeset=2")) {
    bb_log(LOG_WARNING, "couldn't load nouveau\n");
    return;
  }

  /* Find card's device node by enumerating through existing cards. */
  dp = opendir("/sys/class/drm");
  if (dp == NULL) {
    bb_log(LOG_WARNING, "couldn't open /sys/class/drm: %s\n", strerror(errno));
    return;
  }

  while ((ep = readdir(dp))) {
    int candidate_no;
    char dummy;
    // We use dummy to determine if name is "cardN" or "cardN-something" and skip the latter.
    int read_count = sscanf(ep->d_name, "card%i%c", &candidate_no, &dummy);
    if (read_count == 1) {
      char* driver;
      ssize_t link_size = 0;

      snprintf(buf, sizeof(buf), "/sys/class/drm/card%i/device/driver", candidate_no);
      link_size = readlink(buf, buf, sizeof(buf));
      if (link_size < 0 || link_size == sizeof(buf)) {
        bb_log(LOG_DEBUG, "couldn't read driver link for card %i\n", candidate_no);
        continue;
      }
      buf[link_size] = '\0';
      // We get a path like "../../../../bus/pci/drivers/driver_name" and want to test if driver is nouveau.
      driver = strrchr(buf, '/');
      if(driver == NULL) {
        continue;
      }
      if(strcmp(driver + 1, "nouveau") == 0) {
        card_no = candidate_no;
      }
    }
  }

  if(closedir(dp) != 0) {
    bb_log(LOG_WARNING, "couldn't close /sys/class/drm: %s\n", strerror(errno));
  }

  if(card_no == -1) {
    bb_log(LOG_WARNING, "couldn't find discrete card handled by nouveau\n");
    return;
  }

  /* Open nouveau device and place exclusive lock to prevent Xorg from using it */
  snprintf(buf, sizeof(buf), "/dev/dri/card%i", card_no);
  bb_log(LOG_DEBUG, "found nouveau device: %s\n", buf);
  /* udev needs time to create a device node */
  for(; open_retries != 0; open_retries--) {
    nouveau_fd = open(buf, 0);
    if (nouveau_fd != -1) {
      break;
    } else {
      usleep(open_retry_delay_us);
    }
  };

  if (nouveau_fd == -1) {
    bb_log(LOG_WARNING, "couldn't open nouveau device: %s\n", strerror(errno));
    return;
  }
  if (flock(nouveau_fd, LOCK_EX) < 0) {
    bb_log(LOG_WARNING, "couldn't exclusively lock nouveau device: %s\n", strerror(errno));
    return;
  }

  bb_log(LOG_DEBUG, "successfully loaded and locked nouveau\n");
}//nouveau_off
