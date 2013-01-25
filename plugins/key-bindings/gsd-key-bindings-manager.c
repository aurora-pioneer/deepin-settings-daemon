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
	GsdKeyBindingsManagerPrivate* _priv = (GsdKeyBindingsManagerPrivate*) user_data;
	char* _string = g_settings_get_string (settings, gsettings_key);

	//1. check whether the key  have already been set.
	_priv->gsettings_ht;
	KeysAndCmd* _kandc_ptr = gsd_kb_util_parse_gsettings_value (gsettings_key, _string);
	if (_kandc_ptr == NULL) //remove a previous key bindings.
	{
		g_hash_table_remove (_priv->gsettings_ht, gsettings_key);
		return ;
	}

	g_hash_table_replace (_priv->gsettings_ht, gsettings_key, _kandc_ptr);
	
	//-------------------------------------
#if 0
	//1. check gsettings_ht: key---> command string.
	char* _prev_command_name1 = g_hash_table_lookup (_priv->keybindings_ht,
							 gsettings_key);	
	if (_prev_command_name1 != NULL)
	{
		//remove a keybinding from keybindings_ht.
		g_hash_table_remove (_priv->keybindings_ht, _prev_command_name1);
		//
	}
	
	if (_str == NULL)	
	{
	    return;
	}
	else	//replace a key binding.
	{
	    
	}




	GHashTable* _keybindings_ht = (GHashTable*) user_data;
	char* _str = g_settings_get_string (settings, key);

	if (check_is_default(key))
	{
	    //default key bindings
	}
	else
	{
	    //empty slots:   <command> ';' <keys>
	    char* _tmp = strchr (_str, BINDING_DELIMITER);
	    *_tmp = NULL; //split _str into to two strings.
	    char* _command_name = g_strdup (g_strstrip (_str));
	    char* _keystring = g_strdup (g_strstrip (_tmp+1));

	    g_debug ("keybindings: %s ----> %s", _keystring, _command_name);

	    KeysAndHandler* kandh_ptr = gsd_kb_util_keys_and_handler_new (_command_name);

	    char* _prev_command_name = NULL;
	    KeysAndHandler* _prev_kandh_ptr = NULL;
	    if (g_hash_table_lookup_extended (_keybindings_ht, _command_name, 
					      &_prev_command_name, &_prev_kandh_ptr))
	    {
		//previous set command key bindings.
	    }
	    else
	    {
		g_hash_table_insert (_keybindings_ht, _command, _keybinding);
		keybind_bind (_keybinding, handler, NULL);
	    }
	}
#endif 
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
