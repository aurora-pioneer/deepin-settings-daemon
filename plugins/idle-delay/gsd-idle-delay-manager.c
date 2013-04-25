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

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "gnome-settings-profile.h"
#include "gsd-idle-delay-manager.h"
#include "gsd-idle-delay-watcher.h"
#include "gsd-idle-delay-misc.h"

#define GSD_IDLE_DELAY_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_IDLE_DELAY_MANAGER, GsdIdleDelayManagerPrivate))

struct GsdIdleDelayManagerPrivate
{
	GsdIdleDelayWatcher	*watcher;
	GSettings		*settings;	 //idle-delay gsettings
	GSettings		*xrandr_settings;//xrandr gsettings.
};

enum {
        PROP_0,
};

static void     gsd_idle_delay_manager_class_init  (GsdIdleDelayManagerClass *klass);
static void     gsd_idle_delay_manager_init        (GsdIdleDelayManager      *idle_delay_manager);
static void     gsd_idle_delay_manager_finalize    (GObject                  *object);

G_DEFINE_TYPE (GsdIdleDelayManager, gsd_idle_delay_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

gboolean
gsd_idle_delay_manager_start (GsdIdleDelayManager *manager,
                              GError               **error)
{
        g_debug ("Starting idle delay manager");
        gnome_settings_profile_start (NULL);
        gnome_settings_profile_end (NULL);
        return TRUE;
}

void
gsd_idle_delay_manager_stop (GsdIdleDelayManager *manager)
{
        g_debug ("Stopping idle delay manager");
}

static void
gsd_idle_delay_manager_set_property (GObject        *object,
                                     guint           prop_id,
                               	     const GValue   *value,
                                     GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_idle_delay_manager_get_property (GObject        *object,
                                     guint           prop_id,
                                     GValue         *value,
                                     GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_idle_delay_manager_constructor (GType                  type,
                                    guint                  n_construct_properties,
                                    GObjectConstructParam *construct_properties)
{
        GsdIdleDelayManager      *idle_delay_manager;

        idle_delay_manager = GSD_IDLE_DELAY_MANAGER (G_OBJECT_CLASS (gsd_idle_delay_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (idle_delay_manager);
}

static void
gsd_idle_delay_manager_dispose (GObject *object)
{
        G_OBJECT_CLASS (gsd_idle_delay_manager_parent_class)->dispose (object);
}

static void
gsd_idle_delay_manager_class_init (GsdIdleDelayManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_idle_delay_manager_get_property;
        object_class->set_property = gsd_idle_delay_manager_set_property;
        object_class->constructor = gsd_idle_delay_manager_constructor;
        object_class->dispose = gsd_idle_delay_manager_dispose;
        object_class->finalize = gsd_idle_delay_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdIdleDelayManagerPrivate));
}

static gboolean
watcher_idle_cb (GsdIdleDelayWatcher *watcher, gboolean is_idle, 
		 GsdIdleDelayManager *manager)
{
        gboolean res;

        g_debug ("Idle signal detected: %d", is_idle);

        return TRUE;
}

static gboolean
watcher_idle_notice_cb (GsdIdleDelayWatcher *watcher, gboolean in_effect,
                        GsdIdleDelayManager *manager)
{
        gboolean activation_enabled;
        gboolean handled;

        g_debug ("Idle notice signal detected: %d", in_effect);
        handled = FALSE;
        if (in_effect) 
	{
		g_settings_set_double (manager->priv->xrandr_settings,
				       "brightness", 0.5);
        }
	else 
	{
		g_settings_set_double (manager->priv->xrandr_settings,
				       "brightness", 1.0);
        }

        return TRUE;
}

static void
brightness_changed_cb (GSettings* settings, gchar* key, gpointer user_data)
{
}

static void
time_changed_cb (GSettings* settings, gchar* key, gpointer user_data)
{
}

static void
connect_gsettings_signals (GsdIdleDelayManager *manager)
{
	g_signal_connect (manager->priv->settings, "changed::"IDLE_DELAY_KEY_BRIGHTNESS,
			  G_CALLBACK(brightness_changed_cb), manager);
	g_signal_connect (manager->priv->settings, "changed::"IDLE_DELAY_KEY_TIME,
			  G_CALLBACK(time_changed_cb), manager);
}

static void
disconnect_gsettings_signals (GsdIdleDelayManager *manager)
{
        g_signal_handlers_disconnect_by_func (manager->priv->settings, brightness_changed_cb, manager);
        g_signal_handlers_disconnect_by_func (manager->priv->settings, time_changed_cb, manager);
}

static void
connect_watcher_signals (GsdIdleDelayManager *manager)
{
	g_debug ("connect_watcher_signals");
        g_signal_connect (manager->priv->watcher, "idle-changed",
                          G_CALLBACK (watcher_idle_cb), manager);
        g_signal_connect (manager->priv->watcher, "idle-notice-changed",
                          G_CALLBACK (watcher_idle_notice_cb), manager);
}

static void
disconnect_watcher_signals (GsdIdleDelayManager *manager)
{
        g_signal_handlers_disconnect_by_func (manager->priv->watcher, watcher_idle_cb, manager);
        g_signal_handlers_disconnect_by_func (manager->priv->watcher, watcher_idle_notice_cb, manager);
}

static void
gsd_idle_delay_manager_init (GsdIdleDelayManager *manager)
{
        manager->priv = GSD_IDLE_DELAY_MANAGER_GET_PRIVATE (manager);
	
	manager->priv->xrandr_settings = g_settings_new (XRANDR_SCHEMA);
	manager->priv->settings = g_settings_new (IDLE_DELAY_SCHEMA);
	manager->priv->watcher = gsd_idle_delay_watcher_new ();

	connect_gsettings_signals (manager);
	connect_watcher_signals (manager);

	gsd_idle_delay_watcher_set_active (manager->priv->watcher, TRUE);
}

static void
gsd_idle_delay_manager_finalize (GObject *object)
{
        GsdIdleDelayManager *idle_delay_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_IDLE_DELAY_MANAGER (object));

        idle_delay_manager = GSD_IDLE_DELAY_MANAGER (object);

        g_return_if_fail (idle_delay_manager->priv != NULL);
	
	g_object_unref (idle_delay_manager->priv->xrandr_settings);

	disconnect_gsettings_signals (idle_delay_manager);
	g_object_unref (idle_delay_manager->priv->settings);

        disconnect_watcher_signals (idle_delay_manager);
        g_object_unref (idle_delay_manager->priv->watcher);

        G_OBJECT_CLASS (gsd_idle_delay_manager_parent_class)->finalize (object);
}

GsdIdleDelayManager *
gsd_idle_delay_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_IDLE_DELAY_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_IDLE_DELAY_MANAGER (manager_object);
}
