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

#pragma once

#define PCI_VENDOR_ID_NVIDIA  0x10de
#define PCI_CLASS_DISPLAY_VGA 0x0300

struct pci_bus_id {
  unsigned char bus; /* 0x00 - 0xFF */
  unsigned char slot; /* 0x00 - 0x1F */
  unsigned char func; /* 0 - 7 */
};

int pci_parse_bus_id(char *str);
int pci_stringify_bus_id(char *dest, int bus_id);
int pci_get_class(char *bus_id);
int pci_find_gfx_by_vendor(int vendor_id);
size_t pci_get_driver(char *dest, int bus_id, size_t len);
