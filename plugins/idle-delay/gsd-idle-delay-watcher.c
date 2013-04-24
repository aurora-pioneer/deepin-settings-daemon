/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Linux Deepin Inc.
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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include <string.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "gsd-idle-delay-dbus.h"
#include "gsd-idle-delay-marshal.h"
#include "gsd-idle-delay-watcher.h"

static void     gsd_idle_delay_watcher_class_init (GsdIdleDelayWatcherClass *klass);
static void     gsd_idle_delay_watcher_init       (GsdIdleDelayWatcher      *watcher);
static void     gsd_idle_delay_watcher_finalize   (GObject        	    *object);

#define GSD_IDLE_DELAY_WATCHER_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_IDLE_DELAY_WATCHER, GsdIdleDelayWatcherPrivate))

struct GsdIdleDelayWatcherPrivate
{
        /* settingsd_idle_delay */
        guint           enabled : 1;  //enable or disable Idle detection
        guint           delta_notice_timeout;

        /* state */
        guint           active : 1;
        guint           idle : 1;
        guint           idle_notice : 1;

        guint           idle_id;

        GDBusProxy	*presence_proxy; //gnome session presence dbus
};

enum {
        PROP_0,
};

enum {
        IDLE_CHANGED,
        IDLE_NOTICE_CHANGED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (GsdIdleDelayWatcher, gsd_idle_delay_watcher, G_TYPE_OBJECT)

static void
gsd_idle_delay_watcher_get_property (GObject    *object,
                         	     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
        GsdIdleDelayWatcher *self = GSD_IDLE_DELAY_WATCHER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_idle_delay_watcher_set_property (GObject          *object,
                         	     guint             prop_id,
                         	     const GValue     *value,
                         	     GParamSpec       *pspec)
{
        GsdIdleDelayWatcher *self = GSD_IDLE_DELAY_WATCHER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_idle_delay_watcher_class_init (GsdIdleDelayWatcherClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gsd_idle_delay_watcher_finalize;
        object_class->get_property = gsd_idle_delay_watcher_get_property;
        object_class->set_property = gsd_idle_delay_watcher_set_property;

        signals [IDLE_CHANGED] =
                g_signal_new ("idle-changed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsdIdleDelayWatcherClass, idle_changed),
                              NULL,
                              NULL,
                              gsd_idle_delay_marshal_BOOLEAN__BOOLEAN,
                              G_TYPE_BOOLEAN,
                              1, G_TYPE_BOOLEAN);
        signals [IDLE_NOTICE_CHANGED] =
                g_signal_new ("idle-notice-changed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsdIdleDelayWatcherClass, idle_notice_changed),
                              NULL,
                              NULL,
                              gsd_idle_delay_marshal_BOOLEAN__BOOLEAN,
                              G_TYPE_BOOLEAN,
                              1, G_TYPE_BOOLEAN);

        g_type_class_add_private (klass, sizeof (GsdIdleDelayWatcherPrivate));
}

static gboolean
_gsd_idle_delay_watcher_set_session_idle_notice (GsdIdleDelayWatcher *watcher,
                                     		 gboolean   in_effect)
{
        gboolean res = FALSE;

        if (in_effect != watcher->priv->idle_notice) 
	{
                g_signal_emit (watcher, signals [IDLE_NOTICE_CHANGED], 0, in_effect, &res);
                if (res) 
		{
                        g_debug ("Changing idle notice state: %d", in_effect);
                        watcher->priv->idle_notice = in_effect;
                } 
		else 
		{
                        g_debug ("Idle notice signal not handled: %d", in_effect);
                }
        }

        return res;
}

static gboolean
_gsd_idle_delay_watcher_set_session_idle (GsdIdleDelayWatcher *watcher,
                              		  gboolean   is_idle)
{
        gboolean res = FALSE;

        if (is_idle != watcher->priv->idle) 
	{
                g_signal_emit (watcher, signals [IDLE_CHANGED], 0, is_idle, &res);
                if (res) 
		{
                        g_debug ("Changing idle state: %d", is_idle);
                        watcher->priv->idle = is_idle;
                } 
		else 
		{
                        g_debug ("Idle changed signal not handled: %d", is_idle);
                }
        }

        return res;
}

gboolean
gsd_idle_delay_watcher_get_active (GsdIdleDelayWatcher *watcher)
{
        g_return_val_if_fail (GSD_IS_IDLE_DELAY_WATCHER (watcher), FALSE);
        return watcher->priv->active;
}

static void
_gsd_idle_delay_watcher_reset_state (GsdIdleDelayWatcher *watcher)
{
        watcher->priv->idle = FALSE;
        watcher->priv->idle_notice = FALSE;
}

static gboolean
_gsd_idle_delay_watcher_set_active_internal (GsdIdleDelayWatcher *watcher,
                                 	     gboolean   active)
{
        if (active != watcher->priv->active) 
	{
                /* reset state */
                _gsd_idle_delay_watcher_reset_state (watcher);
                watcher->priv->active = active;
        }

        return TRUE;
}

gboolean
gsd_idle_delay_watcher_set_active (GsdIdleDelayWatcher *watcher,
                       		   gboolean   active)
{
        g_return_val_if_fail (GSD_IS_IDLE_DELAY_WATCHER (watcher), FALSE);
        g_debug ("turning watcher: %s", active ? "ON" : "OFF");

        if (watcher->priv->active == active) 
	{
                g_debug ("Idle detection is already %s", active ? "active" : "inactive");
                return FALSE;
        }
        if (! watcher->priv->enabled) 
	{
                g_debug ("Idle detection is disabled, cannot activate");
                return FALSE;
        }

        return _gsd_idle_delay_watcher_set_active_internal (watcher, active);
}

gboolean
gsd_idle_delay_watcher_set_enabled (GsdIdleDelayWatcher *watcher,
                        gboolean   enabled)
{
        g_return_val_if_fail (GSD_IS_IDLE_DELAY_WATCHER (watcher), FALSE);

        if (watcher->priv->enabled != enabled) 
	{
                gboolean is_active = gsd_idle_delay_watcher_get_active (watcher);

                watcher->priv->enabled = enabled;

                /* if we are disabling the watcher and we are
                   active shut it down */
                if (! enabled && is_active) 
		{
                        _gsd_idle_delay_watcher_set_active_internal (watcher, FALSE);
                }
        }

        return TRUE;
}

gboolean
gsd_idle_delay_watcher_get_enabled (GsdIdleDelayWatcher *watcher)
{
        g_return_val_if_fail (GSD_IS_IDLE_DELAY_WATCHER (watcher), FALSE);

        return watcher->priv->enabled;
}

static gboolean
on_idle_timeout (GsdIdleDelayWatcher *watcher)
{
	g_debug ("on idle timeout");
        gboolean res;

        res = _gsd_idle_delay_watcher_set_session_idle (watcher, TRUE);

        _gsd_idle_delay_watcher_set_session_idle_notice (watcher, FALSE);

        /* try again if we failed i guess */
        return !res;
}

static void
set_status (GsdIdleDelayWatcher *watcher,
            guint      status)
{
	g_debug ("set status: %d", status);
        gboolean is_idle;

        if (! watcher->priv->active) 
	{
                g_debug ("GsdIdleDelayWatcher: not active, ignoring status changes");
                return;
        }

        is_idle = (status == 3);

        if (!is_idle && !watcher->priv->idle_notice) 
	{
                /* no change in idleness */
                return;
        }

        if (is_idle) 
	{
                _gsd_idle_delay_watcher_set_session_idle_notice (watcher, is_idle);
                /* queue an activation */
                if (watcher->priv->idle_id > 0) {
                        g_source_remove (watcher->priv->idle_id);
                }
                watcher->priv->idle_id = g_timeout_add (watcher->priv->delta_notice_timeout,
                                                        (GSourceFunc)on_idle_timeout,
                                                        watcher);
        }
	else 
	{
                /* cancel notice too */
                if (watcher->priv->idle_id > 0) 
		{
                        g_source_remove (watcher->priv->idle_id);
                }
                _gsd_idle_delay_watcher_set_session_idle (watcher, FALSE);
                _gsd_idle_delay_watcher_set_session_idle_notice (watcher, FALSE);
        }
}

static void
on_presence_status_changed (GDBusProxy		*presence_proxy,
                            gchar		*send_name,
			    gchar		*signal_name,
			    GVariant		*parameters,
                            GsdIdleDelayWatcher *watcher)
{
	g_debug ("on presence status changed");

	guint status;
	g_variant_get (parameters, "(u)", &status);

        set_status (watcher, status);
}

static void
connect_presence_watcher (GsdIdleDelayWatcher *watcher)
{
	g_debug ("connect presence watcher");

        GError *error;

	//1. connect to gnome-session presence.
        error = NULL;
        watcher->priv->presence_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
								       G_DBUS_PROXY_FLAGS_NONE,
								       NULL,
								       GSM_SERVICE,
								       GSM_PRESENCE_PATH,
								       GSM_PRESENCE_INTERFACE,
								       NULL,
                                                                       &error);
        if (error != NULL) 
	{
                g_warning ("Couldn't connect to session presence : %s", error->message);
                g_error_free (error);
                return;
	}
	g_signal_connect (watcher->priv->presence_proxy,
			  "g-signal",
			  G_CALLBACK (on_presence_status_changed),
			  watcher);
	//2. get initial status value.
	GDBusProxy* _proxy;
	GVariant*   _result;
        error = NULL;
        _proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
						G_DBUS_PROXY_FLAGS_NONE,
						NULL,
						GSM_SERVICE,
						GSM_PRESENCE_PATH,
						"org.freedesktop.DBus.Properties",
						NULL,
                                                &error);
	if (error != NULL) 
	{
                g_warning ("Couldn't connect to session properties: %s", error->message);
                g_error_free (error);
                return;
	}
        error = NULL;
	_result = g_dbus_proxy_call_sync (_proxy, 
					  "Get",
					  g_variant_new ("(ss)", GSM_PRESENCE_INTERFACE, "status"),
					  G_DBUS_CALL_FLAGS_NONE,
					  -1,
					  NULL,
					  &error);
	g_object_unref (_proxy);
	if (error != NULL) 
	{
                g_warning ("Couldn't get session properties: %s", error->message);
                g_error_free (error);
                return;
	}

	GVariant* _tmp;
	g_variant_get (_result, "(v)", &_tmp);

        guint status = g_variant_get_uint32 (_tmp);
	g_variant_unref (_tmp);
	g_variant_unref (_result);
        set_status (watcher, status);
}

static void
gsd_idle_delay_watcher_init (GsdIdleDelayWatcher *watcher)
{
        watcher->priv = GSD_IDLE_DELAY_WATCHER_GET_PRIVATE (watcher);

        watcher->priv->enabled = TRUE;
        watcher->priv->active = FALSE;

        connect_presence_watcher (watcher);

        /* time before idle signal to send notice signal */
        watcher->priv->delta_notice_timeout = 10000;
}

static void
gsd_idle_delay_watcher_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_IDLE_DELAY_WATCHER (object));

        GsdIdleDelayWatcher *watcher = GSD_IDLE_DELAY_WATCHER (object);

        g_return_if_fail (watcher->priv != NULL);

        if (watcher->priv->idle_id > 0) 
                g_source_remove (watcher->priv->idle_id);

        watcher->priv->active = FALSE;

        if (watcher->priv->presence_proxy != NULL) 
                g_object_unref (watcher->priv->presence_proxy);

        G_OBJECT_CLASS (gsd_idle_delay_watcher_parent_class)->finalize (object);
}

GsdIdleDelayWatcher *
gsd_idle_delay_watcher_new (void)
{
        GsdIdleDelayWatcher *watcher;
        watcher = g_object_new (GSD_TYPE_IDLE_DELAY_WATCHER, NULL);

        return GSD_IDLE_DELAY_WATCHER (watcher);
}
