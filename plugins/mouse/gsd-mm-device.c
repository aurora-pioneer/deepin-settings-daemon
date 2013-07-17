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

#include <math.h>

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

#include "gsd-mm-device.h"
#include "gsd-mm-mouse.h"
#include "gsd-mm-touchpad.h"
#include "gsd-mm-trackpoint.h"

static void     device_added_cb         (GdkDeviceManager *device_manager, GdkDevice *device, GsdMouseManager *manager);
static void     device_removed_cb       (GdkDeviceManager *device_manager, GdkDevice *device, GsdMouseManager *manager);
static gboolean device_is_blacklisted   (GsdMouseManager *manager, GdkDevice *device);
static gboolean device_has_buttons      (GdkDevice *device);

void
setup_device_manager (GsdMouseManager *manager)
{
    GdkDeviceManager *device_manager;

    device_manager = gdk_display_get_device_manager (gdk_display_get_default ());
    manager->priv->device_manager = device_manager;

    manager->priv->device_added_id = g_signal_connect (G_OBJECT (device_manager), "device-added",
                                                       G_CALLBACK (device_added_cb), manager);
    manager->priv->device_removed_id = g_signal_connect (G_OBJECT (device_manager), "device-removed",
                                                         G_CALLBACK (device_removed_cb), manager);
}

gboolean
device_is_ignored (GsdMouseManager *manager, GdkDevice *device)
{
    if (device_is_blacklisted (manager, device))
        return TRUE;

    GdkInputSource source = gdk_device_get_source (device);
    if (source != GDK_SOURCE_MOUSE &&
        source != GDK_SOURCE_TOUCHPAD &&
        source != GDK_SOURCE_CURSOR)
        return TRUE;

    const char *name = gdk_device_get_name (device);
    if (name != NULL && g_str_equal ("Virtual core XTEST pointer", name))
        return TRUE;

    return FALSE;
}

static void
device_added_cb (GdkDeviceManager *device_manager, GdkDevice *device, GsdMouseManager *manager)
{
    if (device_is_ignored (manager, device) == FALSE)
    {
        if (run_custom_command (device, COMMAND_DEVICE_ADDED) == FALSE) /*see gsd-input-helper.c*/
        {
            device_apply_settings (manager, device);
        }
        else /* we should not apply any more settings */
        {
            int id;
            g_object_get (G_OBJECT (device), "device-id", &id, NULL);
            g_hash_table_insert (manager->priv->blacklist, GINT_TO_POINTER (id), GINT_TO_POINTER (1));
        }

        /* If a touchpad was to appear... */
        set_disable_w_typing (manager, g_settings_get_boolean (manager->priv->touchpad_settings, KEY_TOUCHPAD_DISABLE_W_TYPING));
    }
}

static void
device_removed_cb (GdkDeviceManager *device_manager, GdkDevice *device, GsdMouseManager *manager)
{
    int id;

    /* Remove the device from the hash table so that
    * device_is_ignored () doesn't check for blacklisted devices */
    g_object_get (G_OBJECT (device), "device-id", &id, NULL);
    g_hash_table_remove (manager->priv->blacklist, GINT_TO_POINTER (id));

    if (device_is_ignored (manager, device) == FALSE)
    {
        run_custom_command (device, COMMAND_DEVICE_REMOVED);

        /* If a touchpad was to disappear... */
        set_disable_w_typing (manager, g_settings_get_boolean (manager->priv->touchpad_settings, KEY_TOUCHPAD_DISABLE_W_TYPING));

        touchpad_ensure_active (manager);
    }
}

static gboolean
device_is_blacklisted (GsdMouseManager *manager, GdkDevice *device)
{
    int id;
    g_object_get (G_OBJECT (device), "device-id", &id, NULL);

    if (g_hash_table_lookup (manager->priv->blacklist, GINT_TO_POINTER (id)) != NULL)
    {
        g_debug ("device %s (%d) is blacklisted", gdk_device_get_name (device), id);
        return TRUE;
    }

    return FALSE;
}

/*
 *  Get a XDevice* from a GdkDevice*
 *  NOTE: you need to manually close the device by XCloseDevice after using it.
 */
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

void
device_set_motion (GsdMouseManager *manager, GdkDevice *device, GSettings* settings)
{
    XDevice *xdevice;
    XPtrFeedbackControl feedback;
    XFeedbackState *states, *state;
    int num_feedbacks;
    int numerator, denominator;
    gfloat motion_acceleration;
    int motion_threshold;
    guint i;

    xdevice = open_gdk_device (device);
    if (xdevice == NULL)
        return;

    g_debug ("setting motion on %s", gdk_device_get_name (device));

    /* Calculate acceleration */
    motion_acceleration = g_settings_get_double (settings, KEY_MOTION_ACCELERATION);

    if (motion_acceleration >= 1.0)
    {
        //we want to get the acceleration, with a resolution of 0.5
        if ((motion_acceleration - floor (motion_acceleration)) < 0.25)
        {
            numerator = floor (motion_acceleration);
            denominator = 1;
        }
        else if ((motion_acceleration - floor (motion_acceleration)) < 0.5)
        {
            numerator = ceil (2.0 * motion_acceleration);
            denominator = 2;
        }
        else if ((motion_acceleration - floor (motion_acceleration)) < 0.75)
        {
            numerator = floor (2.0 *motion_acceleration);
            denominator = 2;
        }
        else
        {
            numerator = ceil (motion_acceleration);
            denominator = 1;
        }
    }
    else if (motion_acceleration < 1.0 && motion_acceleration > 0)
    {
        /* This we do to 1/10ths */
        numerator = floor (motion_acceleration * 10) + 1;
        denominator= 10;
    }
    else
    {
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
    for (i = 0; i < num_feedbacks; i++)
    {
        if (state->class == PtrFeedbackClass)
        {
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
configure_button_layout (guchar *buttons, gint n_buttons, gboolean left_handed)
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
    if (left_handed && buttons[left_button - 1] == left_button)
    {
        /* find the right button */
        for (i = 0; i < n_buttons; i++)
        {
            if (buttons[i] == right_button)
            {
                buttons[i] = left_button;
                break;
            }
        }
        /* swap the buttons */
        buttons[left_button - 1] = right_button;
    }
    /* check if we are not left_handed but are swapped */
    else if (!left_handed && buttons[left_button - 1] == right_button)
    {
        /* find the right button */
        for (i = 0; i < n_buttons; i++)
        {
            if (buttons[i] == left_button)
            {
                buttons[i] = right_button;
                break;
            }
        }
        /* swap the buttons */
        buttons[left_button - 1] = left_button;
    }
}

void
device_set_left_handed (GsdMouseManager *manager, GdkDevice *device, gboolean left_handed)
{
    XDevice *xdevice;
    guchar *buttons;
    gsize buttons_capacity = 16;
    gint n_buttons;

    if (!device_has_buttons (device))
        return;

    xdevice = open_gdk_device (device);
    if (xdevice == NULL)
        return;

    g_debug ("setting handedness on %s", gdk_device_get_name (device));

    buttons = g_new (guchar, buttons_capacity);

    n_buttons = XGetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                         buttons,
                                         buttons_capacity);

    while (n_buttons > buttons_capacity)
    {
        buttons_capacity = n_buttons;
        buttons = (guchar *) g_realloc (buttons, buttons_capacity * sizeof (guchar));

        n_buttons = XGetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice,
                                             buttons,
                                             buttons_capacity);
    }

    configure_button_layout (buttons, n_buttons, left_handed);

    gdk_error_trap_push ();
    XSetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice, buttons, n_buttons);
    gdk_error_trap_pop_ignored ();

    XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
    g_free (buttons);
}

static gboolean
device_has_buttons (GdkDevice *device)
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
    for (i = 0; i < n_devices; i++)
    {
        if (device_info[i].id == id)
        {
            info = &device_info[i];
            break;
        }
    }
    if (info == NULL)
        goto bail;

    class_info = info->inputclassinfo;
    for (i = 0; i < info->num_classes; i++)
    {
        if (class_info->class == ButtonClass)
        {
            XButtonInfo *button_info;

            button_info = (XButtonInfo *) class_info;
            if (button_info->num_buttons > 0)
            {
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
/*
 * FIXME: we should XI2 here, but most of gnome-settings-daemon code
 * still use the deprecated XI API, we'll stick to it until we can
 * afford the time to overhaul it.
 * MOST of the device types can be determined by this function, but
 * for devices like TRACKPOINT we use a hacky way.
 */
static Atom
xdevice_get_type (XDevice* xdevice)
{
    Atom device_type = None;

    XDeviceInfo *device_infos = NULL;
    int n_devices = 0;
    device_infos = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);
    int i = 0;
    for(i = 0; i < n_devices; i++)
    {
        if (device_infos[i].id == xdevice->device_id)
        {
            device_type = device_infos[i].type;
            break;
        }
    }
    XFreeDeviceList (device_infos);
    return device_type;
}

static gboolean
device_is_mouse (XDevice* xdevice)
{
    Atom mouse_type, device_type;

    mouse_type = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), XI_MOUSE, False);
    device_type = xdevice_get_type (xdevice);

    return (mouse_type == device_type);
}

#if 0
//device_is_touchpad is defined in gsd-input-helper.c
static gboolean
device_is_touchpad (XDevice* xdevice)
{
}
#endif

static gboolean
device_is_trackpoint (XDevice* xdevice)
{
    /*
     * there's no XI_TRACKPOINT(the only nearest device is XI_TRACKBALL), so
     * we use a rather hacky way here
     */
    Atom prop = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), "TPPS/2 IBM TrackPoint", False);
    if (!prop)
        return FALSE;

    gdk_error_trap_push ();
    Atom realtype;
    int realformat;
    unsigned long nitems, bytes_after;
    unsigned char *data;
    if ((XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice, prop, 0, 1, False,
                            XA_INTEGER, &realtype, &realformat, &nitems,
                            &bytes_after, &data) == Success) && (realtype != None)) {
        gdk_error_trap_pop_ignored ();
        XFree (data);
        return TRUE;
    }
    gdk_error_trap_pop_ignored ();

    return FALSE;
}

GsdMMDeviceType
device_get_type (GdkDevice* device)
{
    GsdMMDeviceType device_type = GSD_MM_DEVICE_TYPE_UNKNOWN;

    XDevice* xdevice = open_gdk_device (device);
    if (device_is_mouse (xdevice))
    {
        device_type = GSD_MM_DEVICE_TYPE_MOUSE;
        goto out;
    }
    if (device_is_touchpad (xdevice))
    {
        device_type = GSD_MM_DEVICE_TYPE_TOUCHPAD;
        goto out;
    }
    if (device_is_trackpoint (xdevice))
    {
        device_type = GSD_MM_DEVICE_TYPE_TRACKPOINT;
        goto out;
    }
out:
    XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), xdevice);
    return device_type;
}

void
device_apply_settings (GsdMouseManager *manager, GdkDevice *device)
{
    GsdMMDeviceType device_type = device_get_type (device);
    switch (device_type)
    {
        case GSD_MM_DEVICE_TYPE_MOUSE:
            mouse_apply_settings (manager, device);
            break;
        case GSD_MM_DEVICE_TYPE_TOUCHPAD:
            touchpad_apply_settings (manager, device);
            break;
        case GSD_MM_DEVICE_TYPE_TRACKPOINT:
            trackpoint_apply_settings (manager, device);
            break;
        case GSD_MM_DEVICE_TYPE_UNKNOWN:
        default:
            break;
    }
}
