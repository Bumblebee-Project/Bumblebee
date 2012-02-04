/*
 * Copyright (C) 2012 Bumblebee Project
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

#include <glib/gtypes.h>

#define DBUS_SERVICE_NAME "org.bumblebee_project.bumblebeed"

int bb_dbus_init(void);

int bb_dbus_fini(void);

void bb_dbus_set_xorg_pid(gint pid);
void bb_dbus_set_clients_count(gint count);
void bb_dbus_set_card_state(gboolean state);