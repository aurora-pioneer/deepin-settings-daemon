/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Red Hat, Inc.
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

#ifndef __GSD_A11Y_SETTINGS_PLUGIN_H__
#define __GSD_A11Y_SETTINGS_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include "gnome-settings-plugin.h"

G_BEGIN_DECLS

#define GSD_TYPE_A11Y_SETTINGS_PLUGIN                (gsd_a11y_settings_plugin_get_type ())
#define GSD_A11Y_SETTINGS_PLUGIN(o)                  (G_TYPE_CHECK_INSTANCE_CAST ((o), GSD_TYPE_A11Y_SETTINGS_PLUGIN, GsdA11ySettingsPlugin))
#define GSD_A11Y_SETTINGS_PLUGIN_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST((k), GSD_TYPE_A11Y_SETTINGS_PLUGIN, GsdA11ySettingsPluginClass))
#define GSD_IS_A11Y_SETTINGS_PLUGIN(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSD_TYPE_A11Y_SETTINGS_PLUGIN))
#define GSD_IS_A11Y_SETTINGS_PLUGIN_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE ((k), GSD_TYPE_A11Y_SETTINGS_PLUGIN))
#define GSD_A11Y_SETTINGS_PLUGIN_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), GSD_TYPE_A11Y_SETTINGS_PLUGIN, GsdA11ySettingsPluginClass))

typedef struct GsdA11ySettingsPluginPrivate GsdA11ySettingsPluginPrivate;

typedef struct
{
        GnomeSettingsPlugin           parent;
        GsdA11ySettingsPluginPrivate *priv;
} GsdA11ySettingsPlugin;

typedef struct
{
        GnomeSettingsPluginClass parent_class;
} GsdA11ySettingsPluginClass;

GType   gsd_a11y_settings_plugin_get_type            (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_gnome_settings_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __GSD_A11Y_SETTINGS_PLUGIN_H__ */
