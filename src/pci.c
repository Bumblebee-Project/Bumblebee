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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include "pci.h"
#include <stdlib.h>
#include <string.h>
#include "bblogger.h"

/**
 * Builds a Bus ID like 02:f0.1 from a binary representation
 * @param dest The struct to store the Bus ID in
 * @param bus_id The binary Bus ID
 * @return 1 if bus_id is valid, 0 otherwise
 */
int pci_parse_bus_id(struct pci_bus_id *dest, int bus_id_numeric) {
  if (bus_id_numeric >= 0 && bus_id_numeric < 0x10000) {
    dest->bus = bus_id_numeric >> 8;
    dest->slot = (bus_id_numeric >> 3) & 0x1f;
    dest->func = bus_id_numeric & 0x7;
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
int pci_get_class(struct pci_bus_id *bus_id) {
  /* the Bus ID is always of fixed length */
  char class_path[40];
  FILE *fp;

  snprintf(class_path, sizeof class_path,
          "/sys/bus/pci/devices/0000:%02x:%02x.%o/class", bus_id->bus,
          bus_id->slot, bus_id->func);
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
 * @param index A numeric index (the card we want when we have many of a vendor)
 * @return A bus ID struct if a device was found, NULL if no device is found or
 * no memory could be allocated
 */
struct pci_bus_id *pci_find_gfx_by_vendor(unsigned int vendor_id, unsigned int idx) {
  FILE *fp;
  char buf[512];
  unsigned int bus_id_numeric, vendor_device;
  struct pci_bus_id *result;

  fp = fopen("/proc/bus/pci/devices", "r");
  if (!fp) {
    return NULL;
  }

  result = malloc(sizeof (struct pci_bus_id));
  if (!result) {
    return NULL;
  }

  while (fgets(buf, sizeof(buf) - 1, fp)) {
    if (sscanf(buf, "%x %x", &bus_id_numeric, &vendor_device) != 2) {
      continue;
    }
    /* VVVVDDDD becomes VVVV */
    if (vendor_device >> 0x10 == vendor_id) {
      if (pci_parse_bus_id(result, bus_id_numeric)) {
        int pci_class = pci_get_class(result);
        if (pci_class == PCI_CLASS_DISPLAY_VGA ||
                pci_class == PCI_CLASS_DISPLAY_3D) {
          /* yay, found device. Now get next, or clean up and return */
          if (idx--) {
            /* It's not yet our device */
            continue;
          }
          fclose(fp);
          return result;
        }
      }
    }
  }
  /* no device found, clean up and return */
  fclose(fp);
  free(result);
  return NULL;
}

/**
 * Gets the driver name for a given Bus ID. If dest is not null and len is
 * larger than 0, the driver name will be stored in dest
 * @param dest An optional buffer to store the found driver name in
 * @param bus_id A pci_bus_id struct containing a Bus ID
 * @param len The maximum number of bytes to store in dest
 * @return The length of the driver name (which may be larger than len if the
 * buffer was too small) or 0 on error
 */
size_t pci_get_driver(char *dest, struct pci_bus_id *bus_id, size_t len) {
  char path[1024];
  ssize_t read_bytes;
  char *name;

  /* if the bus_id was invalid */
  if (!bus_id) {
    return 0;
  }

  /* the path to the driver if one is loaded */
  snprintf(path, sizeof path,  "/sys/bus/pci/devices/0000:%02x:%02x.%o/driver",
          bus_id->bus, bus_id->slot, bus_id->func);
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
    dest[len - 1] = 0;
  }

  return strlen(name);
}

/**
 * Opens a stream to the PCI configuration space
 * @param bus_id A pci_bus_id struct containing a Bus ID
 * @param mode The mode for opening the file: O_WRONLY for writing, O_RDONLY for
 * reading
 * @return a file handle on success, or NULL on failure
 */
static int pci_config_open(struct pci_bus_id *bus_id, mode_t mode) {
  char config_path[41];

  snprintf(config_path, sizeof config_path,
          "/sys/bus/pci/devices/0000:%02x:%02x.%o/config", bus_id->bus,
          bus_id->slot, bus_id->func);
  return open(config_path, mode);
}

/**
 * Reads data from the PCI configuration space of a device. Based on
 * pci_save_state in drivers/pci/pci.c
 * @param bus_id A pci_bus_id struct containing a Bus ID
 * @param pcs A struct containing the PCI configuration state
 * @return zero on success, non-zero on failure
 */
int pci_config_save(struct pci_bus_id *bus_id, struct pci_config_state *pcs) {
  int i, is_saved = 1;
  int fd = pci_config_open(bus_id, O_RDONLY);
  if (fd == -1) {
    return errno;
  }
  bb_log(LOG_DEBUG, "Saving PCI configuration space...\n");
  for (i = 0; i < 16; i++) {
    /* read 32 bits (8 bytes) from the PCI configuration space */
    if (read(fd, &pcs->saved_config_space[i], 4) != 4) {
      bb_log(LOG_WARNING, "failed to retrieve config space value at offset"
                " %#x - aborting\n", i);
      is_saved = 0;
      break;
    }
    /* Vendor ID and Device ID with all bits enabled is invalid and returned if
     * a device is disabled */
    if (i == 0 && pcs->saved_config_space[i] == (int32_t)-1) {
      bb_log(LOG_WARNING, "invalid device state, is the discrete video card"
              " disabled?\n");
      is_saved = 0;
      break;
    }
  }
  close(fd);
  pcs->state_saved = is_saved;
  return 0;
}

/**
 * Writes data to the PCI configuration space of a device. Based on
 * pci_restore_state in drivers/pci/pci.c
 * @param bus_id A pci_bus_id struct containing a Bus ID
 * @param pcs A struct containing the PCI configuration state
 * @return zero on success, non-zero on failure
 */
int pci_config_restore(struct pci_bus_id *bus_id, struct pci_config_state *pcs) {
  int i;
  int fd_write, fd_read;
  if (!pcs->state_saved) {
    /* nothing to restore, so success? */
    bb_log(LOG_DEBUG, "there is no PCI configuration space to restore\n");
    return 0;
  }
  fd_write = pci_config_open(bus_id, O_WRONLY);
  if (fd_write == -1) {
    return errno;
  }
  fd_read = pci_config_open(bus_id, O_RDONLY);
  if (fd_read == -1) {
    int err = errno;
    close(fd_write);
    return err;
  }

  bb_log(LOG_DEBUG, "Restoring PCI configuration space...\n");

  for (i = 15; i >= 0; i--) {
    int32_t val;

    lseek(fd_read, 4 * i, SEEK_SET);
    if (read(fd_read, &val, 4) == 0) {
      bb_log(LOG_WARNING, "failed to retrieve config space value at offset"
              " %#x - not writing\n", i);
    } else if (val != pcs->saved_config_space[i]) {
      bb_log(LOG_DEBUG, "restoring config space at offset %#x (was %#x, writing"
              " %#x)\n", i, val, pcs->saved_config_space[i]);
      lseek(fd_write, 4 * i, SEEK_SET);

      int written_bytes = write(fd_write, &pcs->saved_config_space[i], 4) != 4;
      if (written_bytes != 4) {
        /* this is unlikely to happen, but not sure if it will never happen */
        bb_log(LOG_WARNING, "The PCI config space could not be written fully at"
                " offset %#x; %i bytes have been written; error: %s",
                i, written_bytes, strerror(errno));
      }
    }
  }
  close(fd_read);
  close(fd_write);
  pcs->state_saved = 0;
  return 0;
}
