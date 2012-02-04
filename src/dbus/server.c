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

#include <glib-2.0/glib/gtypes.h>
#include <glib-2.0/glib/gmessages.h>

#include "dbus.h"
#include "dbus-bumblebeed.h"

static guint owner_id;
static BbdDBusBumblebeed *interface;

static void on_bus_acquired(GDBusConnection *connection, const gchar *name,
        gpointer user_data) {
  GError *error = NULL;

  g_print ("Acquired a message bus connection\n");

  interface = bbd_dbus_bumblebeed_skeleton_new();

  bbd_dbus_bumblebeed_set_card_state(interface, TRUE);
  bbd_dbus_bumblebeed_set_clients_count(interface, 0);
  bbd_dbus_bumblebeed_set_xorg_pid(interface, 0);
  g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(interface),
          connection, "/bumblebeed", &error);

}

static void on_name_acquired(GDBusConnection *connection, const gchar *name,
        gpointer user_data) {
  g_print("Acquired the name %s on the session bus\n", name);
}

static void on_name_lost(GDBusConnection *connection, const gchar *name,
        gpointer user_data) {
  if (!connection) {
    g_print("Could not get a connection with dbus for name %s\n", name);
  } else {
    g_print("Lost the name %s on the session bus\n", name);
  }
}

int bb_dbus_init(void) {
  GBusNameOwnerFlags flags;

  g_type_init();

  flags = G_BUS_NAME_OWNER_FLAGS_NONE;
  flags |= G_BUS_NAME_OWNER_FLAGS_REPLACE;

  owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
          DBUS_SERVICE_NAME,
          flags,
          on_bus_acquired,
          on_name_acquired,
          on_name_lost,
          NULL,
          NULL);
  return 0;
}

int bb_dbus_fini(void) {
  g_bus_unown_name(owner_id);
  return 0;
}

void bb_dbus_set_xorg_pid(gint pid) {
  if (interface) {
    bbd_dbus_bumblebeed_set_xorg_pid(interface, pid);
  }
}
void bb_dbus_set_clients_count(gint count) {
  if (interface) {
    bbd_dbus_bumblebeed_set_clients_count(interface, count);
  }
}
void bb_dbus_set_card_state(gboolean state) {
  if (interface) {
    bbd_dbus_bumblebeed_set_card_state(interface, state);
  }
#ifdef DEBUG_BBD_DBUS
int main(int argc, char *argv[]) {
  GMainLoop *loop;
  bb_dbus_init();
  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
  bb_dbus_fini();
  return 0;
}
#endif
