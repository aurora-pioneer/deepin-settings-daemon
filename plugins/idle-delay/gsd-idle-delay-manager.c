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

#include <X11/extensions/dpms.h>  //used for poweroff display

#include "gnome-settings-profile.h"
#include "gsd-idle-delay-manager.h"
#include "gsd-idle-delay-watcher.h"
#include "gsd-idle-delay-misc.h"

#define GSD_IDLE_DELAY_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_IDLE_DELAY_MANAGER, GsdIdleDelayManagerPrivate))

struct GsdIdleDelayManagerPrivate
{
	GsdIdleDelayWatcher	*watcher;
	GSettings		*settings;	 //idle-delay gsettings
	double			settings_brigthness;
	guint			settings_timeout;
	guint			timeout_id;
	GSettings		*xrandr_settings;//xrandr gsettings.
	//X stuff for DPMS
	Display*		x11_display;
	gboolean		dpms_supported;
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
        g_debug ("Idle signal detected: %d", is_idle);

        return TRUE;
}

static gboolean
on_timeout_cb (gpointer user_data)
{
	g_debug ("on_timeout_cb called");
	GsdIdleDelayManager* manager = GSD_IDLE_DELAY_MANAGER(user_data);
	//turn off the screen
	if (manager->priv->dpms_supported)
	{
		DPMSForceLevel (manager->priv->x11_display, DPMSModeOff);
	}
	else
	{
		g_settings_set_double (manager->priv->xrandr_settings,
			       "brightness", 0.1);
	}
	//never call it again.
	manager->priv->timeout_id = 0;
	return FALSE;
}

static gboolean
watcher_idle_notice_cb (GsdIdleDelayWatcher *watcher, gboolean in_effect,
                        GsdIdleDelayManager *manager)
{
        g_debug ("Idle notice signal detected: %d", in_effect);

	if (manager->priv->timeout_id > 0)
	{
		g_source_remove (manager->priv->timeout_id);
		manager->priv->timeout_id = 0;
	}

        if (in_effect) 
	{
		g_settings_set_double (manager->priv->xrandr_settings,
				       "brightness", manager->priv->settings_brigthness);
		manager->priv->timeout_id = g_timeout_add (manager->priv->settings_timeout*MSEC_PER_SEC,
							   on_timeout_cb,
							   manager);
        }
	else 
	{
		if (manager->priv->dpms_supported)
		{
			DPMSForceLevel (manager->priv->x11_display, DPMSModeOn);
		}
		g_settings_set_double (manager->priv->xrandr_settings,
				       "brightness", 1.0);
        }

        return TRUE;
}

static void
brightness_changed_cb (GSettings* settings, gchar* key, gpointer user_data)
{
	if (g_strcmp0 (key, IDLE_DELAY_KEY_BRIGHTNESS))
		return;

	GsdIdleDelayManager* manager = GSD_IDLE_DELAY_MANAGER(user_data);
	manager->priv->settings_brigthness = g_settings_get_double (manager->priv->settings,
								    IDLE_DELAY_KEY_BRIGHTNESS);
	g_debug ("brightness changed: %lf", manager->priv->settings_brigthness);
}

static void
timeout_changed_cb (GSettings* settings, gchar* key, gpointer user_data)
{
	if (g_strcmp0 (key, IDLE_DELAY_KEY_TIMEOUT))
		return;

	GsdIdleDelayManager* manager = GSD_IDLE_DELAY_MANAGER(user_data);
	manager->priv->settings_timeout = g_settings_get_uint (manager->priv->settings,
							       IDLE_DELAY_KEY_TIMEOUT);
	g_debug ("timeout changed   : %u", manager->priv->settings_timeout);
}

static void
connect_gsettings_signals (GsdIdleDelayManager *manager)
{
	g_signal_connect (manager->priv->settings, "changed::"IDLE_DELAY_KEY_BRIGHTNESS,
			  G_CALLBACK(brightness_changed_cb), manager);
	g_signal_connect (manager->priv->settings, "changed::"IDLE_DELAY_KEY_TIMEOUT,
			  G_CALLBACK(timeout_changed_cb), manager);
}

static void
disconnect_gsettings_signals (GsdIdleDelayManager *manager)
{
        g_signal_handlers_disconnect_by_func (manager->priv->settings, brightness_changed_cb, manager);
        g_signal_handlers_disconnect_by_func (manager->priv->settings, timeout_changed_cb, manager);
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
init_manager_gsettings_value (GsdIdleDelayManager *manager)
{
	manager->priv->settings_brigthness = g_settings_get_double (manager->priv->settings,
								    IDLE_DELAY_KEY_BRIGHTNESS);
	manager->priv->settings_timeout = g_settings_get_uint (manager->priv->settings,
							       IDLE_DELAY_KEY_TIMEOUT);
	manager->priv->timeout_id = 0;
	g_debug ("initial brightness: %lf", manager->priv->settings_brigthness);
	g_debug ("initial timeout   : %u", manager->priv->settings_timeout);
}

static void
gsd_idle_delay_manager_init (GsdIdleDelayManager *manager)
{
        manager->priv = GSD_IDLE_DELAY_MANAGER_GET_PRIVATE (manager);
	
	manager->priv->xrandr_settings = g_settings_new (XRANDR_SCHEMA);
	manager->priv->settings = g_settings_new (IDLE_DELAY_SCHEMA);
	manager->priv->watcher = gsd_idle_delay_watcher_new ();

	init_manager_gsettings_value (manager);
	connect_gsettings_signals (manager);
	connect_watcher_signals (manager);

	gsd_idle_delay_watcher_set_active (manager->priv->watcher, TRUE);
	
	//initialize DPMS
	Display* _display;
	_display = gdk_x11_display_get_xdisplay (gdk_display_get_default());
	int tmp1, tmp2;
	gboolean has_dmps;
	if (DPMSQueryExtension (_display, &tmp1, &tmp2) == TRUE)
	{
		has_dmps = TRUE;

		DPMSGetVersion (_display, &tmp1, &tmp2);
		g_debug ("DPMS extension detected: major=%d, minor=%d", tmp1, tmp2);
		DPMSEnable (_display);
	}
	else
	{
		g_warning ("Your X server doesn't support DPMS extension, which "
			   "is required for power off your screen");
		has_dmps = FALSE;
	}
	
	manager->priv->x11_display = _display;
	manager->priv->dpms_supported = has_dmps;
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
	
	if (idle_delay_manager->priv->timeout_id != 0)
		g_source_remove (idle_delay_manager->priv->timeout_id);


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
