/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Linux Deepin Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
#include "config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#ifdef __linux
#include <sys/prctl.h>
#endif

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>

#include "gsd-mouse-manager.h"
#include "gsd-input-helper.h"
#include "gsd-enums.h"

#include "gsd-mm-device.h"
#include "gsd-mm-mouse.h"

void
mouse_apply_settings (GsdMouseManager *manager, GdkDevice *device)
{
    gboolean mouse_left_handed;
    mouse_left_handed = g_settings_get_boolean (manager->priv->mouse_settings, KEY_LEFT_HANDED);

    device_set_left_handed (manager, device, mouse_left_handed);
    device_set_motion (manager, device, manager->priv->mouse_settings);

    set_middle_button (manager, device, g_settings_get_boolean (manager->priv->mouse_settings, KEY_MIDDLE_BUTTON_EMULATION));
}

void
set_middle_button (GsdMouseManager *manager,
                   GdkDevice       *device,
                   gboolean         middle_button)
{
        Atom prop;
        XDevice *xdevice;
        Atom type;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        int rc;

        prop = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                            "Evdev Middle Button Emulation", True);

        if (!prop) /* no evdev devices */
                return;

        xdevice = open_gdk_device (device);
        if (xdevice == NULL)
                return;

	g_debug ("setting middle button on %s", gdk_device_get_name (device));

        gdk_error_trap_push ();

        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                            xdevice, prop, 0, 1, False, XA_INTEGER, &type, &format,
                            &nitems, &bytes_after, &data);

        if (rc == Success && format == 8 && type == XA_INTEGER && nitems == 1) {
                data[0] = middle_button ? 1 : 0;

                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                       xdevice, prop, type, format, PropModeReplace, data, nitems);
        }

        if (gdk_error_trap_pop ())
                g_warning ("Error in setting middle button emulation on \"%s\"", gdk_device_get_name (device));

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
}

void
set_mousetweaks_daemon (GsdMouseManager *manager,
                        gboolean         dwell_click_enabled,
                        gboolean         secondary_click_enabled)
{
        GError *error = NULL;
        gchar *comm;
        gboolean run_daemon = dwell_click_enabled || secondary_click_enabled;

        if (run_daemon || manager->priv->mousetweaks_daemon_running)
                comm = g_strdup_printf ("mousetweaks %s",
                                        run_daemon ? "" : "-s");
        else
                return;

        if (run_daemon)
                manager->priv->mousetweaks_daemon_running = TRUE;

        if (! g_spawn_command_line_async (comm, &error)) {
                if (error->code == G_SPAWN_ERROR_NOENT && run_daemon) {
                        GtkWidget *dialog;

                        if (dwell_click_enabled) {
                                g_settings_set_boolean (manager->priv->mouse_a11y_settings,
                                                        KEY_DWELL_CLICK_ENABLED, FALSE);
                        } else if (secondary_click_enabled) {
                                g_settings_set_boolean (manager->priv->mouse_a11y_settings,
                                                        KEY_SECONDARY_CLICK_ENABLED, FALSE);
                        }

                        dialog = gtk_message_dialog_new (NULL, 0,
                                                         GTK_MESSAGE_WARNING,
                                                         GTK_BUTTONS_OK,
                                                         _("Could not enable mouse accessibility features"));
                        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                                  _("Mouse accessibility requires Mousetweaks "
                                                                    "to be installed on your system."));
                        gtk_window_set_title (GTK_WINDOW (dialog), _("Universal Access"));
                        gtk_window_set_icon_name (GTK_WINDOW (dialog),
                                                  "preferences-desktop-accessibility");
                        gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                }
                g_error_free (error);
        }
        g_free (comm);
}


void
mouse_callback (GSettings       *settings,
                const gchar     *key,
                GsdMouseManager *manager)
{
        GList *devices, *l;

        if (g_str_equal (key, KEY_DWELL_CLICK_ENABLED) ||
            g_str_equal (key, KEY_SECONDARY_CLICK_ENABLED)) {
                set_mousetweaks_daemon (manager,
                                        g_settings_get_boolean (settings, KEY_DWELL_CLICK_ENABLED),
                                        g_settings_get_boolean (settings, KEY_SECONDARY_CLICK_ENABLED));
                return;
        } else if (g_str_equal (key, KEY_LOCATE_POINTER)) {
                set_locate_pointer (manager, g_settings_get_boolean (settings, KEY_LOCATE_POINTER));
                return;
        }

        devices = gdk_device_manager_list_devices (manager->priv->device_manager, GDK_DEVICE_TYPE_SLAVE);

        for (l = devices; l != NULL; l = l->next) {
                GdkDevice *device = l->data;

                if (device_is_ignored (manager, device))
                        continue;

                if (g_str_equal (key, KEY_LEFT_HANDED)) {
                        gboolean mouse_left_handed;
                        mouse_left_handed = g_settings_get_boolean (settings, KEY_LEFT_HANDED);
                        device_set_left_handed (manager, device, mouse_left_handed);
                } else if (g_str_equal (key, KEY_MOTION_ACCELERATION) ||
                           g_str_equal (key, KEY_MOTION_THRESHOLD)) {
                        device_set_motion (manager, device, manager->priv->mouse_settings);
                } else if (g_str_equal (key, KEY_MIDDLE_BUTTON_EMULATION)) {
                        set_middle_button (manager, device, g_settings_get_boolean (settings, KEY_MIDDLE_BUTTON_EMULATION));
                }
        }
        g_list_free (devices);
}

void
set_locate_pointer (GsdMouseManager *manager,
                    gboolean         state)
{
        if (state) {
                GError *error = NULL;
                char *args[2];

                if (manager->priv->locate_pointer_spawned)
                        return;

                args[0] = LIBEXECDIR "/gsd-locate-pointer";
                args[1] = NULL;

                g_spawn_async (NULL, args, NULL,
                               0, NULL, NULL,
                               &manager->priv->locate_pointer_pid, &error);

                manager->priv->locate_pointer_spawned = (error == NULL);

                if (error) {
                        g_settings_set_boolean (manager->priv->mouse_settings, KEY_LOCATE_POINTER, FALSE);
                        g_error_free (error);
                }

        } else if (manager->priv->locate_pointer_spawned) {
                kill (manager->priv->locate_pointer_pid, SIGHUP);
                g_spawn_close_pid (manager->priv->locate_pointer_pid);
                manager->priv->locate_pointer_spawned = FALSE;
        }
}


