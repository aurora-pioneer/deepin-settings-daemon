/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Linux Deepin Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include "gnome-settings-plugin.h"
#include "gsd-idledelay-plugin.h"
#include "gsd-idledelay-manager.h"

struct GsdIdledelayPluginPrivate {
        GsdIdledelayManager *manager;
};

#define GSD_IDLEDELAY_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GSD_TYPE_IDLEDELAY_PLUGIN, GsdIdledelayPluginPrivate))

GNOME_SETTINGS_PLUGIN_REGISTER (GsdIdledelayPlugin, gsd_idledelay_plugin)

static void
gsd_idledelay_plugin_init (GsdIdledelayPlugin *plugin)
{
        plugin->priv = GSD_IDLEDELAY_PLUGIN_GET_PRIVATE (plugin);

        g_debug ("GsdIdledelayPlugin initializing");

        plugin->priv->manager = gsd_idledelay_manager_new ();
}

static void
gsd_idledelay_plugin_finalize (GObject *object)
{
        GsdIdledelayPlugin *plugin;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_IDLEDELAY_PLUGIN (object));

        g_debug ("GsdIdledelayPlugin finalizing");

        plugin = GSD_IDLEDELAY_PLUGIN (object);

        g_return_if_fail (plugin->priv != NULL);

        if (plugin->priv->manager != NULL) {
                g_object_unref (plugin->priv->manager);
        }

        G_OBJECT_CLASS (gsd_idledelay_plugin_parent_class)->finalize (object);
}

static void
impl_activate (GnomeSettingsPlugin *plugin)
{
        gboolean res;
        GError  *error;

        g_debug ("Activating idledelay plugin");

        error = NULL;
        res = gsd_idledelay_manager_start (GSD_IDLEDELAY_PLUGIN (plugin)->priv->manager, &error);
        if (! res) {
                g_warning ("Unable to start idledelay manager: %s", error->message);
                g_error_free (error);
        }
}

static void
impl_deactivate (GnomeSettingsPlugin *plugin)
{
        g_debug ("Deactivating idledelay plugin");
        gsd_idledelay_manager_stop (GSD_IDLEDELAY_PLUGIN (plugin)->priv->manager);
}

static void
gsd_idledelay_plugin_class_init (GsdIdledelayPluginClass *klass)
{
        GObjectClass           *object_class = G_OBJECT_CLASS (klass);
        GnomeSettingsPluginClass *plugin_class = GNOME_SETTINGS_PLUGIN_CLASS (klass);

        object_class->finalize = gsd_idledelay_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (klass, sizeof (GsdIdledelayPluginPrivate));
}

static void
gsd_idledelay_plugin_class_finalize (GsdIdledelayPluginClass *klass)
{
}

