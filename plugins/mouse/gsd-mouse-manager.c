/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#include <gdk/gdk.h>

#include "gnome-settings-profile.h"
#include "gsd-mouse-manager.h"
#include "gsd-input-helper.h"
#include "gsd-enums.h"

#include "gsd-mm-device.h"
#include "gsd-mm-mouse.h"
#include "gsd-mm-touchpad.h"
#include "gsd-mm-trackpoint.h"

#define GSD_MOUSE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_MOUSE_MANAGER, GsdMouseManagerPrivate))

static void     gsd_mouse_manager_class_init  (GsdMouseManagerClass *klass);
static void     gsd_mouse_manager_init        (GsdMouseManager *mouse_manager);
static void     gsd_mouse_manager_finalize    (GObject *object);

G_DEFINE_TYPE (GsdMouseManager, gsd_mouse_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static GObject *
gsd_mouse_manager_constructor (GType type,
                               guint n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
    GsdMouseManager *mouse_manager;

    mouse_manager = GSD_MOUSE_MANAGER (G_OBJECT_CLASS (gsd_mouse_manager_parent_class)->constructor (type,
                                                                                                  n_construct_properties,
                                                                                                  construct_properties));

    return G_OBJECT (mouse_manager);
}

static void
gsd_mouse_manager_dispose (GObject *object)
{
    G_OBJECT_CLASS (gsd_mouse_manager_parent_class)->dispose (object);
}

static void
gsd_mouse_manager_class_init (GsdMouseManagerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->constructor = gsd_mouse_manager_constructor;
    object_class->dispose = gsd_mouse_manager_dispose;
    object_class->finalize = gsd_mouse_manager_finalize;

    g_type_class_add_private (klass, sizeof (GsdMouseManagerPrivate));
}

static void
gsd_mouse_manager_init (GsdMouseManager *manager)
{
    manager->priv = GSD_MOUSE_MANAGER_GET_PRIVATE (manager);

    manager->priv->blacklist = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static gboolean
gsd_mouse_manager_idle_cb (GsdMouseManager *manager)
{
    gnome_settings_profile_start (NULL);

    setup_device_manager (manager);

    mouse_init_settings (manager);
    touchpad_init_settings (manager);
    trackpoint_init_settings (manager);

    //apply settings to the corresponding devices.
    GList *devices, *l;
    devices = gdk_device_manager_list_devices (manager->priv->device_manager, GDK_DEVICE_TYPE_SLAVE);
    for (l = devices; l != NULL; l = l->next)
    {
        GdkDevice *device = l->data;

        if (device_is_ignored (manager, device))
            continue;

        if (run_custom_command (device, COMMAND_DEVICE_PRESENT) == FALSE)
        {
            device_apply_settings (manager, device);
        }
        else/* otherwise blacklist the device */
        {
            int id;
            g_object_get (G_OBJECT (device), "device-id", &id, NULL);
            g_hash_table_insert (manager->priv->blacklist, GINT_TO_POINTER (id), GINT_TO_POINTER (1));
        }
    }
    g_list_free (devices);

    //additional touchpad settings
    touchpad_ensure_active (manager);
    if (g_settings_get_boolean (manager->priv->touchpad_settings, KEY_TOUCHPAD_ENABLED))
    {
        devices = get_disabled_devices (manager->priv->device_manager);
        for (l = devices; l != NULL; l = l->next)
        {
            int device_id;
            device_id = GPOINTER_TO_INT (l->data);
            touchpad_set_enabled (device_id);
        }
        g_list_free (devices);
    }

    gnome_settings_profile_end (NULL);

    manager->priv->start_idle_id = 0;

    return FALSE;
}

gboolean
gsd_mouse_manager_start (GsdMouseManager *manager,
                         GError         **error)
{
    gnome_settings_profile_start (NULL);

    if (!supports_xinput_devices ())
    {
        g_debug ("XInput is not supported, not applying any settings");
        return TRUE;
    }

    manager->priv->start_idle_id = g_idle_add ((GSourceFunc) gsd_mouse_manager_idle_cb, manager);

    gnome_settings_profile_end (NULL);

    return TRUE;
}

void
gsd_mouse_manager_stop (GsdMouseManager *manager)
{
    GsdMouseManagerPrivate *p = manager->priv;

    g_debug ("Stopping mouse manager");

    if (manager->priv->start_idle_id != 0)
    {
        g_source_remove (manager->priv->start_idle_id);
        manager->priv->start_idle_id = 0;
    }

    if (p->device_manager != NULL)
    {
        g_signal_handler_disconnect (p->device_manager, p->device_added_id);
        g_signal_handler_disconnect (p->device_manager, p->device_removed_id);
        p->device_manager = NULL;
    }

    g_clear_object (&p->mouse_a11y_settings);
    g_clear_object (&p->mouse_settings);
    g_clear_object (&p->touchpad_settings);

    mouse_set_locate_pointer (manager, FALSE);
}

static void
gsd_mouse_manager_finalize (GObject *object)
{
    GsdMouseManager *mouse_manager;

    g_return_if_fail (object != NULL);
    g_return_if_fail (GSD_IS_MOUSE_MANAGER (object));

    mouse_manager = GSD_MOUSE_MANAGER (object);

    g_return_if_fail (mouse_manager->priv != NULL);

    gsd_mouse_manager_stop (mouse_manager);

    if (mouse_manager->priv->blacklist != NULL)
        g_hash_table_destroy (mouse_manager->priv->blacklist);

    G_OBJECT_CLASS (gsd_mouse_manager_parent_class)->finalize (object);
}

GsdMouseManager *
gsd_mouse_manager_new (void)
{
    if (manager_object != NULL)
    {
        g_object_ref (manager_object);
    }
    else
    {
        manager_object = g_object_new (GSD_TYPE_MOUSE_MANAGER, NULL);
        g_object_add_weak_pointer (manager_object, (gpointer *) &manager_object);
    }

    return GSD_MOUSE_MANAGER (manager_object);
}
