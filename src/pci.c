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

#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include "pci.h"
#include <stdlib.h>
#include <string.h>

/**
 * Converts a string like 02:00.1 or 02:00.1 (bus:slot.func) to a number which
 * in binary looks like BBBB BBBB SSSS SFFF
 * @param str The string to be converted
 * @return A number representating the PCI Bus ID or -1 if an invalid value is
 * given
 */
int pci_parse_bus_id(char *str) {
  int bus, slot, func;
  /* match hex 0x00 - 0xFF, followed by a colon, followed by another 0x00 - 0x1F
   * finishing with either a dot or colon and a 0 - 7
   */
  if (sscanf(str, "%2x:%2x%*[:.]%1o", &bus, &slot, &func) == 3) {
    if (slot <= 0x1F) {
      return (bus << 8) + (slot << 3) + func;
    }
  }
  return -1;
}

/**
 * Builds a Bus ID like 02:f0.1 from a binary representation
 * @param dest The string to store the Bus ID in with a size of at least 8
 * @param bus_id The binary Bus ID
 * @return 1 if bus_id is valid, 0 otherwise
 */
int pci_stringify_bus_id(char *dest, int bus_id) {
  if (bus_id >= 0 && bus_id < 0x10000) {
    sprintf(dest, "%02x:%02x.%o", bus_id >> 8, (bus_id >> 3) & 0x1f,
            bus_id & 0x7);
    return 1;
  }
  return 0;
}

/**
 * Gets the class of a device given by the Bus ID
 * @param bus_id A string containing a Bus ID like 01:00.0
 * @return The class number of a device as shown by lspci or 0 if the class
 * could not be determined
 */
int pci_get_class(char *bus_id) {
  /* the Bus ID is always of fixed length */
  char class_path[40];
  FILE *fp;

  snprintf(class_path, sizeof class_path,
          "/sys/bus/pci/devices/0000:%s/class", bus_id);
  fp = fopen(class_path, "r");
  if (fp) {
    char class_buff[16];
    int read_bytes;

    read_bytes = fread(class_buff, 1, sizeof class_buff, fp);
    class_buff[read_bytes] = 0;
    fclose(fp);
    return strtol(class_buff, NULL, 0) >> 8;
  }
  return 0;
}

/**
 * Finds the Bus ID a graphics card by vendor ID
 * @param vendor_id A numeric vendor ID
 * @return -1 if no device was found, the Bus ID otherwise
 */
int pci_find_gfx_by_vendor(int vendor_id) {
  FILE *fp;
  char buf[512];
  int bus_id, vendor_device;

  fp = fopen("/proc/bus/pci/devices", "r");
  if (!fp) {
    return -1;
  }

  while (fgets(buf, sizeof(buf) - 1, fp)) {
    if (sscanf(buf, "%x %x", &bus_id, &vendor_device) != 2) {
      continue;
    }
    /* VVVVDDDD becomes VVVV */
    if (vendor_device >> 0x10 == vendor_id) {
      /* the contents of the buffer can be discarded, so just re-use the buffer
       * for other purposes */
      if (pci_stringify_bus_id(buf, bus_id)) {
        if (pci_get_class(buf) == PCI_CLASS_DISPLAY_VGA) {
          /* yay, found device. Now clean up and return */
          fclose(fp);
          return bus_id;
        }
      }
    }
  }
  /* no device found, clean up and return */
  fclose(fp);
  return -1;
}

/**
 * Gets the driver name for a given Bus ID. If dest is not null and len is
 * larger than 0, the driver name will be stored in dest
 * @param dest An optional buffer to store the found driver name in
 * @param bus_id A string containing a Bus ID like 01:00.0
 * @param len The maximum number of bytes to store in dest
 * @return The length of the driver name (which may be larger than len if the
 * buffer was too small) or 0 on error
 */
size_t pci_get_driver(char *dest, int bus_id, size_t len) {
  char bus_id_str[8];
  char path[1024];
  ssize_t read_bytes;
  char *name;

  /* if the bus_id was invalid */
  if (!pci_stringify_bus_id(bus_id_str, bus_id)) {
    return 0;
  }

  /* the path to the driver if one is loaded */
  snprintf(path, sizeof path,  "/sys/bus/pci/devices/0000:%s/driver",
          bus_id_str);
  read_bytes = readlink(path, path, sizeof(path) - 1);
  if (read_bytes < 0) {
    /* error, assume that the driver is not loaded */
    return 0;
  }

  /* readlink does not append a NULL according to the manpage */
  path[read_bytes] = 0;

  name = basename(path);
  /* save the name if a valid destination and buffer size was given */
  if (dest && len > 0) {
    strncpy(dest, name, len - 1);
    dest[len] = 0;
  }

  return strlen(name);
}
