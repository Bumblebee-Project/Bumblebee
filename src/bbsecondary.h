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
#pragma once

/**
 * OpenSUSE: /usr/bin/X -> /var/lib/X11/X -> /usr/bin/Xorg
 * Fedora, Arch Linux: /usr/bin/X -> /usr/bin/Xorg
 * Ubuntu: /usr/bin/X is a custom binary doing authorization and then executes
 *         /etc/X11/X -> /usr/bin/Xorg
 */
#define XORG_BINARY "Xorg"

/* PCI Bus ID of the discrete video card */
struct pci_bus_id *pci_bus_id_discrete;

/// Start the X server by fork-exec, turn card on if needed.
bool start_secondary(bool);

/// Kill the second X server if any, turn card off if requested.
void stop_secondary(void);

/* check for the availability of PM methods */
void check_pm_method(void);
