/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "gnome-settings-profile.h"
#include "gsd-key-bindings-manager.h"

#include "keybinder.h"
#include "parse-super.h"
#include "gsd-key-bindings-handler.h"
#include "gsd-key-bindings-settings.h"
#include "gsd-key-bindings-util.h"

#define GSD_KEY_BINDINGS_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_KEY_BINDINGS_MANAGER, GsdKeyBindingsManagerPrivate))

struct GsdKeyBindingsManagerPrivate
{
        gboolean padding;
	GSettings*  settings;		//setup in nit
	//gsettings key --> command and keys struct
	GHashTable* gsettings_ht;
	//this hashtable is used for conflict detection
	//to ensure that a single key can not be bound to multiple commands.
	GHashTable* keyandcmd_ht;	
};

enum {
        PROP_0,
};

static void     gsd_key_bindings_manager_class_init  (GsdKeyBindingsManagerClass *klass);
static void     gsd_key_bindings_manager_init        (GsdKeyBindingsManager      *key_bindings_manager);
static void     gsd_key_bindings_manager_finalize    (GObject             *object);

G_DEFINE_TYPE (GsdKeyBindingsManager, gsd_key_bindings_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

gboolean
gsd_key_bindings_manager_start (GsdKeyBindingsManager *manager,
                               GError               **error)
{
        g_debug ("Starting key-bindings manager");
        gnome_settings_profile_start (NULL);

        manager->priv = GSD_KEY_BINDINGS_MANAGER_GET_PRIVATE (manager);
	GsdKeyBindingsManagerPrivate* _priv = manager->priv;

	//1. read intial key bindings.
	gsd_kb_util_read_gsettings (_priv->settings, _priv->gsettings_ht);

	//2. check for conflicts.

        gnome_settings_profile_end (NULL);
        return TRUE;
}

void
gsd_key_bindings_manager_stop (GsdKeyBindingsManager *manager)
{
        g_debug ("Stopping key bindings manager");

        manager->priv = GSD_KEY_BINDINGS_MANAGER_GET_PRIVATE (manager);
	GsdKeyBindingsManagerPrivate* _priv = manager->priv;

	//1. clear key bindings.
	g_hash_table_remove_all (_priv->gsettings_ht);
	g_hash_table_remove_all (_priv->keyandcmd_ht);

    /*
     * stop XRecord
     */
    g_debug ("remove all record in stop!\n");
    remove_table_all_record ();
}

static void
gsd_key_bindings_manager_set_property (GObject        *object,
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
gsd_key_bindings_manager_get_property (GObject        *object,
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
gsd_key_bindings_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        GsdKeyBindingsManager      *key_bindings_manager;

        key_bindings_manager = GSD_KEY_BINDINGS_MANAGER (G_OBJECT_CLASS (gsd_key_bindings_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (key_bindings_manager);
}

static void
gsd_key_bindings_manager_dispose (GObject *object)
{
        G_OBJECT_CLASS (gsd_key_bindings_manager_parent_class)->dispose (object);
}

static void
gsd_key_bindings_manager_class_init (GsdKeyBindingsManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_key_bindings_manager_get_property;
        object_class->set_property = gsd_key_bindings_manager_set_property;
        object_class->constructor = gsd_key_bindings_manager_constructor;
        object_class->dispose = gsd_key_bindings_manager_dispose;
        object_class->finalize = gsd_key_bindings_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdKeyBindingsManagerPrivate));
}


static void 
key_bindings_settings_changed (GSettings *settings, gchar *gsettings_key, gpointer user_data)
{	
    if ( settings == NULL || gsettings_key == NULL || 
            user_data == NULL ) {
        return ;
    }

	GsdKeyBindingsManagerPrivate* _priv = (GsdKeyBindingsManagerPrivate*) user_data;
	char* _string = g_settings_get_string (settings, gsettings_key);
	g_debug ("keybindings changed: %s : %s", gsettings_key, _string);

	KeysAndCmd* _kandc_ptr = gsd_kb_util_parse_gsettings_value (gsettings_key, _string);
        g_free(_string);
	//1. new value is NULL, unbind previous keybindings
	if (_kandc_ptr == NULL) //remove a previous key bindings.
	{
		g_hash_table_remove (_priv->gsettings_ht, gsettings_key);
		return ;
	}
	//2. new value is not NULL, unbind previous keybindings.
	//   and bind new keybindings.
	g_hash_table_replace (_priv->gsettings_ht, g_strdup (gsettings_key), _kandc_ptr);

	//3. this is the actual binding operation. this must
	//   follow g_hash_table_replace. 
	KeybinderHandler _handler = gsd_kb_handler_default;

	g_debug ("bind %s -----> %s", _kandc_ptr->keystring, _kandc_ptr->cmdstring);
	keybinder_bind (_kandc_ptr->keystring, _handler, _kandc_ptr->cmdstring);
}

static void
gsd_key_bindings_manager_init (GsdKeyBindingsManager *manager)
{
        manager->priv = GSD_KEY_BINDINGS_MANAGER_GET_PRIVATE (manager);
	GsdKeyBindingsManagerPrivate* _priv = manager->priv;

	//1. connect X server and initialize  keybinder library.
	keybinder_init ();

	//2. init gsettings_key  ----> keysandcmd hashtable.
	_priv->gsettings_ht = g_hash_table_new_full (g_str_hash, g_str_equal,
						     gsd_kb_util_key_free_func, 
						     gsd_kb_util_value_free_func);
						     	
	_priv->keyandcmd_ht = g_hash_table_new (g_direct_hash, g_direct_equal);
	//3.  get GSettings.
	_priv->settings = g_settings_new (KEY_BINDING_SCHEMA_ID);
	g_signal_connect (_priv->settings, "changed",
		          G_CALLBACK (key_bindings_settings_changed), _priv);

}

static void
gsd_key_bindings_manager_finalize (GObject *object)
{
        GsdKeyBindingsManager *key_bindings_manager;
	GsdKeyBindingsManagerPrivate* _priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_KEY_BINDINGS_MANAGER (object));

        key_bindings_manager = GSD_KEY_BINDINGS_MANAGER (object);

	//finalize priv stuff
	_priv = key_bindings_manager->priv;
	//1. disconnect signals
	g_signal_handlers_disconnect_by_func (_priv->settings, 
					      key_bindings_settings_changed,
					      _priv);
	//2. free hashtable and unbind keybindings.
	g_hash_table_destroy (_priv->keyandcmd_ht);
	g_hash_table_destroy (_priv->gsettings_ht);
	
        g_return_if_fail (key_bindings_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_key_bindings_manager_parent_class)->finalize (object);

    /*
     * stop XRecord
     */
    g_debug ("finalize XRecord in finalize!\n");
    finalize_xrecord ();
	destroy_grab_xi2_manager ();
}

GsdKeyBindingsManager *
gsd_key_bindings_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_KEY_BINDINGS_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_KEY_BINDINGS_MANAGER (manager_object);
}
