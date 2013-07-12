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

#include "gsd-common-misc.h"
#include "gsd-touchpad.h"

gboolean
touchpad_has_single_button (XDevice *device)
{
        Atom type, prop;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        gboolean is_single_button = FALSE;
        int rc;

        prop = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "Synaptics Capabilities", False);
        if (!prop)
                return FALSE;

        gdk_error_trap_push ();
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 1, False,
                                XA_INTEGER, &type, &format, &nitems,
                                &bytes_after, &data);
        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 3)
                is_single_button = (data[0] == 1 && data[1] == 0 && data[2] == 0);

        if (rc == Success)
                XFree (data);

        gdk_error_trap_pop_ignored ();

        return is_single_button;
}

/* Ensure that syndaemon dies together with us, to avoid running several of
 * them */
static void
setup_syndaemon (gpointer user_data)
{
#ifdef __linux
        prctl (PR_SET_PDEATHSIG, SIGHUP);
#endif
}

static gboolean
have_program_in_path (const char *name)
{
        gchar *path;
        gboolean result;

        path = g_find_program_in_path (name);
        result = (path != NULL);
        g_free (path);
        return result;
}

static void
syndaemon_died (GPid pid, gint status, gpointer user_data)
{
        GsdMouseManager *manager = GSD_MOUSE_MANAGER (user_data);

        g_debug ("syndaemon stopped with status %i", status);
        g_spawn_close_pid (pid);
        manager->priv->syndaemon_spawned = FALSE;
}

int
set_disable_w_typing (GsdMouseManager *manager, gboolean state)
{
        if (state && touchpad_is_present ()) {
                GError *error = NULL;
                GPtrArray *args;

                if (manager->priv->syndaemon_spawned)
                        return 0;

                if (!have_program_in_path ("syndaemon"))
                        return 0;

                args = g_ptr_array_new ();

                g_ptr_array_add (args, "syndaemon");
                g_ptr_array_add (args, "-i");
                g_ptr_array_add (args, "1.0");
                g_ptr_array_add (args, "-t");
                g_ptr_array_add (args, "-K");
                g_ptr_array_add (args, "-R");
                g_ptr_array_add (args, NULL);

                /* we must use G_SPAWN_DO_NOT_REAP_CHILD to avoid
                 * double-forking, otherwise syndaemon will immediately get
                 * killed again through (PR_SET_PDEATHSIG when the intermediate
                 * process dies */
                g_spawn_async (g_get_home_dir (), (char **) args->pdata, NULL,
                                G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD, setup_syndaemon, NULL,
                                &manager->priv->syndaemon_pid, &error);

                manager->priv->syndaemon_spawned = (error == NULL);
                g_ptr_array_free (args, FALSE);

                if (error) {
                        g_warning ("Failed to launch syndaemon: %s", error->message);
                        g_settings_set_boolean (manager->priv->touchpad_settings, KEY_TOUCHPAD_DISABLE_W_TYPING, FALSE);
                        g_error_free (error);
                } else {
                        g_child_watch_add (manager->priv->syndaemon_pid, syndaemon_died, manager);
                        g_debug ("Launched syndaemon");
                }
        } else if (manager->priv->syndaemon_spawned) {
                kill (manager->priv->syndaemon_pid, SIGHUP);
                g_spawn_close_pid (manager->priv->syndaemon_pid);
                manager->priv->syndaemon_spawned = FALSE;
                g_debug ("Killed syndaemon");
        }

        return 0;
}

void
set_tap_to_click (GdkDevice *device,
                  gboolean   state,
                  gboolean   left_handed)
{
        int format, rc;
        unsigned long nitems, bytes_after;
        XDevice *xdevice;
        unsigned char* data;
        Atom prop, type;

        prop = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "Synaptics Tap Action", False);
        if (!prop)
                return;

        xdevice = open_gdk_device (device);
        if (xdevice == NULL)
                return;

        if (!device_is_touchpad (xdevice)) {
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
                return;
        }

	g_debug ("setting tap to click on %s", gdk_device_get_name (device));

        gdk_error_trap_push ();
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice, prop, 0, 2,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 7) {
                /* Set RLM mapping for 1/2/3 fingers*/
                data[4] = (state) ? ((left_handed) ? 3 : 1) : 0;
                data[5] = (state) ? ((left_handed) ? 1 : 3) : 0;
                data[6] = (state) ? 2 : 0;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice, prop, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        if (gdk_error_trap_pop ())
                g_warning ("Error in setting tap to click on \"%s\"", gdk_device_get_name (device));

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
}

void
set_horiz_scroll (GdkDevice *device,
                  gboolean   state)
{
        int rc;
        XDevice *xdevice;
        Atom act_type, prop_edge, prop_twofinger;
        int act_format;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        prop_edge = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "Synaptics Edge Scrolling", False);
        prop_twofinger = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "Synaptics Two-Finger Scrolling", False);

        if (!prop_edge || !prop_twofinger)
                return;

        xdevice = open_gdk_device (device);
        if (xdevice == NULL)
                return;

        if (!device_is_touchpad (xdevice)) {
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
                return;
        }

	g_debug ("setting horiz scroll on %s", gdk_device_get_name (device));

        gdk_error_trap_push ();
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                 prop_edge, 0, 1, False,
                                 XA_INTEGER, &act_type, &act_format, &nitems,
                                 &bytes_after, &data);
        if (rc == Success && act_type == XA_INTEGER &&
            act_format == 8 && nitems >= 2) {
                data[1] = (state && data[0]);
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                       prop_edge, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }

        XFree (data);

        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                 prop_twofinger, 0, 1, False,
                                 XA_INTEGER, &act_type, &act_format, &nitems,
                                 &bytes_after, &data);
        if (rc == Success && act_type == XA_INTEGER &&
            act_format == 8 && nitems >= 2) {
                data[1] = (state && data[0]);
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                       prop_twofinger, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }

        if (gdk_error_trap_pop ())
                g_warning ("Error in setting horiz scroll on \"%s\"", gdk_device_get_name (device));

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);

}

void
set_edge_scroll (GdkDevice               *device,
                 GsdTouchpadScrollMethod  method)
{
        int rc;
        XDevice *xdevice;
        Atom act_type, prop_edge, prop_twofinger;
        int act_format;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        prop_edge = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "Synaptics Edge Scrolling", False);
        prop_twofinger = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "Synaptics Two-Finger Scrolling", False);

        if (!prop_edge || !prop_twofinger)
                return;

        xdevice = open_gdk_device (device);
        if (xdevice == NULL)
                return;

        if (!device_is_touchpad (xdevice)) {
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
                return;
        }

	g_debug ("setting edge scroll on %s", gdk_device_get_name (device));

        gdk_error_trap_push ();
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                 prop_edge, 0, 1, False,
                                 XA_INTEGER, &act_type, &act_format, &nitems,
                                 &bytes_after, &data);
        if (rc == Success && act_type == XA_INTEGER &&
            act_format == 8 && nitems >= 2) {
                data[0] = (method == GSD_TOUCHPAD_SCROLL_METHOD_EDGE_SCROLLING) ? 1 : 0;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                       prop_edge, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }

        XFree (data);

        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                 prop_twofinger, 0, 1, False,
                                 XA_INTEGER, &act_type, &act_format, &nitems,
                                 &bytes_after, &data);
        if (rc == Success && act_type == XA_INTEGER &&
            act_format == 8 && nitems >= 2) {
                data[0] = (method == GSD_TOUCHPAD_SCROLL_METHOD_TWO_FINGER_SCROLLING) ? 1 : 0;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                       prop_twofinger, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }

        if (gdk_error_trap_pop ())
                g_warning ("Error in setting edge scroll on \"%s\"", gdk_device_get_name (device));

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
}

void
set_touchpad_disabled (GdkDevice *device)
{
        int id;
        XDevice *xdevice;

        g_object_get (G_OBJECT (device), "device-id", &id, NULL);

        g_debug ("Trying to set device disabled for \"%s\" (%d)", gdk_device_get_name (device), id);

        xdevice = open_gdk_device (device);
        if (xdevice == NULL)
                return;

        if (!device_is_touchpad (xdevice)) {
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
                return;
        }

        if (set_device_enabled (id, FALSE) == FALSE)
                g_warning ("Error disabling device \"%s\" (%d)", gdk_device_get_name (device), id);
        else
                g_debug ("Disabled device \"%s\" (%d)", gdk_device_get_name (device), id);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
}

void
set_touchpad_enabled (int id)
{
        XDevice *xdevice;

        g_debug ("Trying to set device enabled for %d", id);

        gdk_error_trap_push ();
        xdevice = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), id);
        if (gdk_error_trap_pop () != 0)
                return;

        if (!device_is_touchpad (xdevice)) {
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
                return;
        }

        if (set_device_enabled (id, TRUE) == FALSE)
                g_warning ("Error enabling device \"%d\"", id);
        else
                g_debug ("Enabled device %d", id);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
}

gboolean
get_touchpad_handedness (GsdMouseManager *manager, gboolean mouse_left_handed)
{
        switch (g_settings_get_enum (manager->priv->touchpad_settings, KEY_LEFT_HANDED)) {
        case GSD_TOUCHPAD_HANDEDNESS_RIGHT:
                return FALSE;
        case GSD_TOUCHPAD_HANDEDNESS_LEFT:
                return TRUE;
        case GSD_TOUCHPAD_HANDEDNESS_MOUSE:
                return mouse_left_handed;
        default:
                g_assert_not_reached ();
        }
}

void
set_natural_scroll (GsdMouseManager *manager,
                    GdkDevice       *device,
                    gboolean         natural_scroll)
{
        XDevice *xdevice;
        Atom scrolling_distance, act_type;
        int rc, act_format;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        glong *ptr;

        xdevice = open_gdk_device (device);
        if (xdevice == NULL)
                return;

        if (!device_is_touchpad (xdevice)) {
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
                return;
        }

        g_debug ("Trying to set %s for \"%s\"",
                 natural_scroll ? "natural (reverse) scroll" : "normal scroll",
                 gdk_device_get_name (device));

        scrolling_distance = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                          "Synaptics Scrolling Distance", False);

        gdk_error_trap_push ();
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                 scrolling_distance, 0, 2, False,
                                 XA_INTEGER, &act_type, &act_format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && act_type == XA_INTEGER && act_format == 32 && nitems >= 2) {
                ptr = (glong *) data;

                if (natural_scroll) {
                        ptr[0] = -abs(ptr[0]);
                        ptr[1] = -abs(ptr[1]);
                } else {
                        ptr[0] = abs(ptr[0]);
                        ptr[1] = abs(ptr[1]);
                }

                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                       scrolling_distance, XA_INTEGER, act_format,
                                       PropModeReplace, data, nitems);
        }

        if (gdk_error_trap_pop ())
                g_warning ("Error setting %s for \"%s\"",
                           natural_scroll ? "natural (reverse) scroll" : "normal scroll",
                           gdk_device_get_name (device));

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
}

void
touchpad_callback (GSettings       *settings,
                   const gchar     *key,
                   GsdMouseManager *manager)
{
        GList *devices, *l;

        if (g_str_equal (key, KEY_TOUCHPAD_DISABLE_W_TYPING)) {
                set_disable_w_typing (manager, g_settings_get_boolean (manager->priv->touchpad_settings, key));
                return;
        }

        devices = gdk_device_manager_list_devices (manager->priv->device_manager, GDK_DEVICE_TYPE_SLAVE);

        for (l = devices; l != NULL; l = l->next) {
                GdkDevice *device = l->data;

                if (device_is_ignored (manager, device))
                        continue;

                if (g_str_equal (key, KEY_TAP_TO_CLICK)) {
                        set_tap_to_click (device, g_settings_get_boolean (settings, key),
                                          g_settings_get_boolean (manager->priv->touchpad_settings, KEY_LEFT_HANDED));
                } else if (g_str_equal (key, KEY_SCROLL_METHOD)) {
                        set_edge_scroll (device, g_settings_get_enum (settings, key));
                        set_horiz_scroll (device, g_settings_get_boolean (settings, KEY_PAD_HORIZ_SCROLL));
                } else if (g_str_equal (key, KEY_PAD_HORIZ_SCROLL)) {
                        set_horiz_scroll (device, g_settings_get_boolean (settings, key));
                } else if (g_str_equal (key, KEY_TOUCHPAD_ENABLED)) {
                        if (g_settings_get_boolean (settings, key) == FALSE)
                                set_touchpad_disabled (device);
                        else
                                set_touchpad_enabled (gdk_x11_device_get_id (device));
                } else if (g_str_equal (key, KEY_MOTION_ACCELERATION) ||
                           g_str_equal (key, KEY_MOTION_THRESHOLD)) {
                        set_motion (manager, device);
                } else if (g_str_equal (key, KEY_LEFT_HANDED)) {
                        gboolean mouse_left_handed;
                        mouse_left_handed = g_settings_get_boolean (manager->priv->mouse_settings, KEY_LEFT_HANDED);
                        set_left_handed (manager, device, mouse_left_handed, get_touchpad_handedness (manager, mouse_left_handed));
                } else if (g_str_equal (key, KEY_NATURAL_SCROLL_ENABLED)) {
                        set_natural_scroll (manager, device, g_settings_get_boolean (settings, key));
                }
        }
        g_list_free (devices);

        if (g_str_equal (key, KEY_TOUCHPAD_ENABLED) &&
            g_settings_get_boolean (settings, key)) {
                devices = get_disabled_devices (manager->priv->device_manager);
                for (l = devices; l != NULL; l = l->next) {
                        int device_id;

                        device_id = GPOINTER_TO_INT (l->data);
                        set_touchpad_enabled (device_id);
                }
                g_list_free (devices);
        }
}

/* Re-enable touchpad when any other pointing device isn't present. */
void
ensure_touchpad_active (GsdMouseManager *manager)
{
        if (mouse_is_present () == FALSE && touchscreen_is_present () == FALSE && trackball_is_present () == FALSE && touchpad_is_present ())
                g_settings_set_boolean (manager->priv->touchpad_settings, KEY_TOUCHPAD_ENABLED, TRUE);
}

