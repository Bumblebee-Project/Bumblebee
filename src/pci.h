/*
 * Copyright (c) 2011-2013, The Bumblebee Project
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

#pragma once
#include <sys/types.h> /* necessary for int32_t */

#define PCI_VENDOR_ID_AMD     0x1002
#define PCI_VENDOR_ID_NVIDIA  0x10de
#define PCI_VENDOR_ID_INTEL   0x8086
#define PCI_CLASS_DISPLAY_VGA 0x0300
#define PCI_CLASS_DISPLAY_3D  0x0302

struct pci_bus_id {
  unsigned char bus; /* 0x00 - 0xFF */
  unsigned char slot; /* 0x00 - 0x1F */
  unsigned char func; /* 0 - 7 */
};

int pci_parse_bus_id(struct pci_bus_id *dest, int bus_id_numeric);
int pci_get_class(struct pci_bus_id *bus_id);
struct pci_bus_id *pci_find_gfx_by_vendor(unsigned int vendor_id, unsigned int idx);
size_t pci_get_driver(char *dest, struct pci_bus_id *bus_id, size_t len);

struct pci_config_state {
    int state_saved;
    int32_t saved_config_space[16];
};

int pci_config_save(struct pci_bus_id *bus_id, struct pci_config_state *pcs);
int pci_config_restore(struct pci_bus_id *bus_id, struct pci_config_state *pcs);
