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

#include "gnome-settings-profile.h"
#include "gsd-mouse-manager.h"
#include "gsd-input-helper.h"
#include "gsd-enums.h"

#include "gsd-common-misc.h"
#include "gsd-touchpad.h"
#include "gsd-mouse.h"

XDevice *
open_gdk_device (GdkDevice *device)
{
        XDevice *xdevice;
        int id;

        g_object_get (G_OBJECT (device), "device-id", &id, NULL);

        gdk_error_trap_push ();

        xdevice = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), id);

        if (gdk_error_trap_pop () != 0)
                return NULL;

        return xdevice;
}

static gboolean
device_is_blacklisted (GsdMouseManager *manager,
                       GdkDevice       *device)
{
        int id;
        g_object_get (G_OBJECT (device), "device-id", &id, NULL);
        if (g_hash_table_lookup (manager->priv->blacklist, GINT_TO_POINTER (id)) != NULL) {
                g_debug ("device %s (%d) is blacklisted", gdk_device_get_name (device), id);
                return TRUE;
        }
        return FALSE;
}

gboolean
device_is_ignored (GsdMouseManager *manager,
		   GdkDevice       *device)
{
	GdkInputSource source;
	const char *name;

	if (device_is_blacklisted (manager, device))
		return TRUE;

	source = gdk_device_get_source (device);
	if (source != GDK_SOURCE_MOUSE &&
	    source != GDK_SOURCE_TOUCHPAD &&
	    source != GDK_SOURCE_CURSOR)
		return TRUE;

	name = gdk_device_get_name (device);
	if (name != NULL && g_str_equal ("Virtual core XTEST pointer", name))
		return TRUE;

	return FALSE;
}

void
set_motion (GsdMouseManager *manager,
            GdkDevice       *device)
{
        XDevice *xdevice;
        XPtrFeedbackControl feedback;
        XFeedbackState *states, *state;
        int num_feedbacks;
        int numerator, denominator;
        gfloat motion_acceleration;
        int motion_threshold;
        GSettings *settings;
        guint i;

        xdevice = open_gdk_device (device);
        if (xdevice == NULL)
                return;

	g_debug ("setting motion on %s", gdk_device_get_name (device));

        if (device_is_touchpad (xdevice))
                settings = manager->priv->touchpad_settings;
        else
                settings = manager->priv->mouse_settings;

        /* Calculate acceleration */
        motion_acceleration = g_settings_get_double (settings, KEY_MOTION_ACCELERATION);

        if (motion_acceleration >= 1.0) {
                /* we want to get the acceleration, with a resolution of 0.5
                 */
                if ((motion_acceleration - floor (motion_acceleration)) < 0.25) {
                        numerator = floor (motion_acceleration);
                        denominator = 1;
                } else if ((motion_acceleration - floor (motion_acceleration)) < 0.5) {
                        numerator = ceil (2.0 * motion_acceleration);
                        denominator = 2;
                } else if ((motion_acceleration - floor (motion_acceleration)) < 0.75) {
                        numerator = floor (2.0 *motion_acceleration);
                        denominator = 2;
                } else {
                        numerator = ceil (motion_acceleration);
                        denominator = 1;
                }
        } else if (motion_acceleration < 1.0 && motion_acceleration > 0) {
                /* This we do to 1/10ths */
                numerator = floor (motion_acceleration * 10) + 1;
                denominator= 10;
        } else {
                numerator = -1;
                denominator = -1;
        }

        /* And threshold */
        motion_threshold = g_settings_get_int (settings, KEY_MOTION_THRESHOLD);

        /* Get the list of feedbacks for the device */
        states = XGetFeedbackControl (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice, &num_feedbacks);
        if (states == NULL)
                goto out;
        state = (XFeedbackState *) states;
        for (i = 0; i < num_feedbacks; i++) {
                if (state->class == PtrFeedbackClass) {
                        /* And tell the device */
                        feedback.class      = PtrFeedbackClass;
                        feedback.length     = sizeof (XPtrFeedbackControl);
                        feedback.id         = state->id;
                        feedback.threshold  = motion_threshold;
                        feedback.accelNum   = numerator;
                        feedback.accelDenom = denominator;

                        g_debug ("Setting accel %d/%d, threshold %d for device '%s'",
                                 numerator, denominator, motion_threshold, gdk_device_get_name (device));

                        XChangeFeedbackControl (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                                xdevice,
                                                DvAccelNum | DvAccelDenom | DvThreshold,
                                                (XFeedbackControl *) &feedback);

                        break;
                }
                state = (XFeedbackState *) ((char *) state + state->length);
        }

        XFreeFeedbackList (states);

    out:

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
}

static void
configure_button_layout (guchar   *buttons,
                         gint      n_buttons,
                         gboolean  left_handed)
{
        const gint left_button = 1;
        gint right_button;
        gint i;

        /* if the button is higher than 2 (3rd button) then it's
         * probably one direction of a scroll wheel or something else
         * uninteresting
         */
        right_button = MIN (n_buttons, 3);

        /* If we change things we need to make sure we only swap buttons.
         * If we end up with multiple physical buttons assigned to the same
         * logical button the server will complain. This code assumes physical
         * button 0 is the physical left mouse button, and that the physical
         * button other than 0 currently assigned left_button or right_button
         * is the physical right mouse button.
         */

        /* check if the current mapping satisfies the above assumptions */
        if (buttons[left_button - 1] != left_button &&
            buttons[left_button - 1] != right_button)
                /* The current mapping is weird. Swapping buttons is probably not a
                 * good idea.
                 */
                return;

        /* check if we are left_handed and currently not swapped */
        if (left_handed && buttons[left_button - 1] == left_button) {
                /* find the right button */
                for (i = 0; i < n_buttons; i++) {
                        if (buttons[i] == right_button) {
                                buttons[i] = left_button;
                                break;
                        }
                }
                /* swap the buttons */
                buttons[left_button - 1] = right_button;
        }
        /* check if we are not left_handed but are swapped */
        else if (!left_handed && buttons[left_button - 1] == right_button) {
                /* find the right button */
                for (i = 0; i < n_buttons; i++) {
                        if (buttons[i] == left_button) {
                                buttons[i] = right_button;
                                break;
                        }
                }
                /* swap the buttons */
                buttons[left_button - 1] = left_button;
        }
}



void
set_left_handed (GsdMouseManager *manager,
                 GdkDevice       *device,
                 gboolean mouse_left_handed,
                 gboolean touchpad_left_handed)
{
        XDevice *xdevice;
        guchar *buttons;
        gsize buttons_capacity = 16;
        gboolean left_handed;
        gint n_buttons;

        if (!xinput_device_has_buttons (device))
                return;

        xdevice = open_gdk_device (device);
        if (xdevice == NULL)
                return;

	g_debug ("setting handedness on %s", gdk_device_get_name (device));

        buttons = g_new (guchar, buttons_capacity);

        /* If the device is a touchpad, swap tap buttons
         * around too, otherwise a tap would be a right-click */
        if (device_is_touchpad (xdevice)) {
                gboolean tap = g_settings_get_boolean (manager->priv->touchpad_settings, KEY_TAP_TO_CLICK);
                gboolean single_button = touchpad_has_single_button (xdevice);

                left_handed = touchpad_left_handed;

                if (tap && !single_button)
                        set_tap_to_click (device, tap, left_handed);

                if (single_button)
                        goto out;
        } else {
                left_handed = mouse_left_handed;
        }

        n_buttons = XGetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                             buttons,
                                             buttons_capacity);

        while (n_buttons > buttons_capacity) {
                buttons_capacity = n_buttons;
                buttons = (guchar *) g_realloc (buttons,
                                                buttons_capacity * sizeof (guchar));

                n_buttons = XGetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                                     buttons,
                                                     buttons_capacity);
        }

        configure_button_layout (buttons, n_buttons, left_handed);

	gdk_error_trap_push ();
        XSetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice, buttons, n_buttons);
        gdk_error_trap_pop_ignored ();

out:
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
        g_free (buttons);
}

gboolean
xinput_device_has_buttons (GdkDevice *device)
{
        int i;
        XAnyClassInfo *class_info;

        /* FIXME can we use the XDevice's classes here instead? */
        XDeviceInfo *device_info, *info;
        gint n_devices;
        int id;

        /* Find the XDeviceInfo for the GdkDevice */
        g_object_get (G_OBJECT (device), "device-id", &id, NULL);

        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);
        if (device_info == NULL)
                return FALSE;

        info = NULL;
        for (i = 0; i < n_devices; i++) {
                if (device_info[i].id == id) {
                        info = &device_info[i];
                        break;
                }
        }
        if (info == NULL)
                goto bail;

        class_info = info->inputclassinfo;
        for (i = 0; i < info->num_classes; i++) {
                if (class_info->class == ButtonClass) {
                        XButtonInfo *button_info;

                        button_info = (XButtonInfo *) class_info;
                        if (button_info->num_buttons > 0) {
                                XFreeDeviceList (device_info);
                                return TRUE;
                        }
                }

                class_info = (XAnyClassInfo *) (((guchar *) class_info) +
                                                class_info->length);
        }

bail:
        XFreeDeviceList (device_info);

        return FALSE;
}

static void
device_added_cb (GdkDeviceManager *device_manager,
                 GdkDevice        *device,
                 GsdMouseManager  *manager)
{
        if (device_is_ignored (manager, device) == FALSE) {
                if (run_custom_command (device, COMMAND_DEVICE_ADDED) == FALSE) {
                        set_mouse_settings (manager, device);
                } else {
                        int id;
                        g_object_get (G_OBJECT (device), "device-id", &id, NULL);
                        g_hash_table_insert (manager->priv->blacklist,
                                             GINT_TO_POINTER (id), GINT_TO_POINTER (1));
                }

                /* If a touchpad was to appear... */
                set_disable_w_typing (manager, g_settings_get_boolean (manager->priv->touchpad_settings, KEY_TOUCHPAD_DISABLE_W_TYPING));
        }
}

static void
device_removed_cb (GdkDeviceManager *device_manager,
                   GdkDevice        *device,
                   GsdMouseManager  *manager)
{
	int id;

	/* Remove the device from the hash table so that
	 * device_is_ignored () doesn't check for blacklisted devices */
	g_object_get (G_OBJECT (device), "device-id", &id, NULL);
	g_hash_table_remove (manager->priv->blacklist,
			     GINT_TO_POINTER (id));

        if (device_is_ignored (manager, device) == FALSE) {
                run_custom_command (device, COMMAND_DEVICE_REMOVED);

                /* If a touchpad was to disappear... */
                set_disable_w_typing (manager, g_settings_get_boolean (manager->priv->touchpad_settings, KEY_TOUCHPAD_DISABLE_W_TYPING));

                ensure_touchpad_active (manager);
        }
}

void
set_devicepresence_handler (GsdMouseManager *manager)
{
        GdkDeviceManager *device_manager;

        device_manager = gdk_display_get_device_manager (gdk_display_get_default ());

        manager->priv->device_added_id = g_signal_connect (G_OBJECT (device_manager), "device-added",
                                                           G_CALLBACK (device_added_cb), manager);
        manager->priv->device_removed_id = g_signal_connect (G_OBJECT (device_manager), "device-removed",
                                                             G_CALLBACK (device_removed_cb), manager);
        manager->priv->device_manager = device_manager;
}


