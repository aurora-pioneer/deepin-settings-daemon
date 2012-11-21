/*
 * Copyright (C) 2012 Deepin, Inc.
 *               2012 Zhai Xiang <zhaixiang@linuxdeepin.com>
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
#include "gsd-helloworld-plugin.h"
#include "gsd-helloworld-manager.h"

struct GsdHelloWorldPluginPrivate {
    GsdHelloWorldManager *manager;
};

#define GSD_HELLOWORLD_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GSD_TYPE_HELLOWORLD_PLUGIN, GsdHelloWorldPluginPrivate))

GNOME_SETTINGS_PLUGIN_REGISTER(GsdHelloWorldPlugin, gsd_helloworld_plugin)

static void gsd_helloworld_plugin_init(GsdHelloWorldPlugin *plugin)
{
    plugin->priv = GSD_HELLOWORLD_PLUGIN_GET_PRIVATE(plugin);

    g_debug("GsdHelloWorldPlugin initializing");

    plugin->priv->manager = gsd_helloworld_manager_new();
}

static void gsd_helloworld_plugin_finalize(GObject *object)
{
    GsdHelloWorldPlugin *plugin;

    g_return_if_fail(object != NULL);
    g_return_if_fail(GSD_IS_HELLOWORLD_PLUGIN(object));

    g_debug("GsdHelloWorldPlugin finalizing");

    plugin = GSD_HELLOWORLD_PLUGIN(object);

    g_return_if_fail(plugin->priv != NULL);

    if (plugin->priv->manager != NULL) {
        g_object_unref(plugin->priv->manager);
    }

    G_OBJECT_CLASS(gsd_helloworld_plugin_parent_class)->finalize(object);
}

static void impl_activate(GnomeSettingsPlugin *plugin)
{
    gboolean res;
    GError  *error;

    g_debug("Activating helloworld plugin");

    error = NULL;
    res = gsd_helloworld_manager_start(GSD_HELLOWORLD_PLUGIN(plugin)->priv->manager, &error);
    if (!res) {
        g_warning("Unable to start helloworld manager: %s", error->message);
        g_error_free(error);
    }
}

static void impl_deactivate(GnomeSettingsPlugin *plugin)
{
    g_debug("Deactivating helloworld plugin");
    gsd_helloworld_manager_stop(GSD_HELLOWORLD_PLUGIN(plugin)->priv->manager);
}

static void gsd_helloworld_plugin_class_init(GsdHelloWorldPluginClass *klass)
{
    GObjectClass           *object_class = G_OBJECT_CLASS(klass);
    GnomeSettingsPluginClass *plugin_class = GNOME_SETTINGS_PLUGIN_CLASS(klass);

    object_class->finalize = gsd_helloworld_plugin_finalize;

    plugin_class->activate = impl_activate;
    plugin_class->deactivate = impl_deactivate;

    g_type_class_add_private(klass, sizeof(GsdHelloWorldPluginPrivate));
}

static void gsd_helloworld_plugin_class_finalize(GsdHelloWorldPluginClass *klass)
{
}
