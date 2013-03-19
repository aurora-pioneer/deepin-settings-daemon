/**
 * Copyright (c) 2011 ~ 2012 Deepin, Inc.
 *               2011 ~ 2012 hooke
 *
 * Author:      hooke
 * Maintainer:  hooke
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 **/
/*
 *	this is used to generate gaussian blurred background image
 *	files for launcher 
 *	and acts as a bridge between different 
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/X.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include "background_util.h"

#define BG_GAUSSIAN_PICT_PATH	"/tmp/.deepin_background_gaussian.png"

/*
 *	how to generate gaussian blurred images:
 *	background plugin (A); gsd-background-helper (B)
 *	A: 1. setup a worklist (ensure that all src_uri are distinct.)
 *	A: 2. use socketpair for bidirectional communication
 *	      pass the other end to gsd-background-helper.
 *	A: 3. pass a src_uri to gsd-background-helper.
 *	A: x. user chooses another background image. A add this src_uri to
 *	      the worklist.
 *	B: 4. generate a blurred image for the src_uri by 3.
 *	      then return OK to A.
 *	A: 5. A receives OK. send another src_uri to gsd-background-helper.
 *	      and remove it from worklist.
 *	A: 6. when the worklist is empty send NULL to terminate gsd-background-helper.
 *
 *	some quick return cases 
 */

//previous picture
static char* prev_pict_path = NULL;
/*
 *	we have a single copy of every src_uri. it can only be freed by g_hash_table_destroy
 *	GPtrArray is act as a queue: old elements are removed from the head; new elements 
 *	are appended to the end.
 *	src_uri_ht: src_uri ---> status (initialized to 0 (on work_list) and set to 1 when 
 *	gsd-background-helper completes this drawing); 
 */
//worklist
//static GPtrArray*   work_list = NULL;
//static GHashTable*  src_uri_ht = NULL; 

//
static void register_account_service_background_path (GsdBackgroundManager* manager, const char* cur_pict);
static void start_gaussian_helper (const char* cur_pict_path);
static void bg_settings_cur_pict_changed (GSettings *settings, gchar *key, gpointer user_data);
static void initial_setup (GsdBackgroundManager* manager);
static gboolean update_prev_pict_path (const char* cur_pict_path);

/*
 *	return: FALSE, we don't need to update.
 *	        TRUE,  we need to update 
 */
static gboolean
update_prev_pict_path (const char* cur_pict_path)
{
    if (!g_strcmp0 (prev_pict_path, cur_pict_path))
    {
	//no need to generate pictures.
	if (prev_pict_path!=NULL)
	{
	   g_print ("start_gaussian_helper: alread started for this picture: %s\n", prev_pict_path);
	}
	return FALSE;
    }
    else
    {
	g_free (prev_pict_path);
	prev_pict_path = NULL;
	prev_pict_path = g_strdup (cur_pict_path);

	return TRUE;
    }
}
/*
 *	start gaussina helper in the background
 */
static void
start_gaussian_helper (const char* cur_pict_path)
{
    //quick return 1. 
    if (cur_pict_path == NULL)
	return ;

    unlink (BG_GAUSSIAN_PICT_PATH);
    (void)symlink (cur_pict_path, BG_GAUSSIAN_PICT_PATH);

    //LIBEXECDIR is a CPP macro. see Makefile.am
    char* command = NULL;

#if 1
    command = g_strdup_printf (LIBEXECDIR "/gsd-background-helper "
			       "%lf %lu %s",
			       BG_GAUSSIAN_SIGMA, BG_GAUSSIAN_NSTEPS, cur_pict_path);
#else 
    //for testing locally.
    command = g_strdup_printf ("./gsd-background-helper "
			       "%lf %lu %s",
			       BG_GAUSSIAN_SIGMA, BG_GAUSSIAN_NSTEPS, cur_pict_path);
#endif 

    g_debug ("command : %s", command);

    GError *error = NULL;
    gboolean ret = FALSE;
    ret = g_spawn_command_line_async (command, &error);
    if (ret == FALSE) 
    {
	g_debug ("Failed to launch '%s': %s", command, error->message);
	g_error_free (error);
    }

    g_debug ("gsd-background-helper started");
    g_free (command);
}

/*
 *	parse picture-uris string and
 *	add them to global array---picture_paths
 *
 *	<picture_uris> := (<uri> ";")* <uri> [";"]
 */
#if 0
static void
parse_picture_uris (gchar * pic_uri)
{
    gchar* uri_end;   // end of a uri
    gchar* uri_start;   //start of a uri
    gchar* filename_ptr;

    uri_start = pic_uri;
    while ((uri_end = strchr (uri_start, DELIMITER)) != NULL)
    {
	*uri_end = '\0';
	
       	filename_ptr = g_filename_from_uri (uri_start, NULL, NULL);
	if (filename_ptr != NULL)
	{
	    g_ptr_array_add (picture_paths, filename_ptr);
	    picture_num ++;
	    g_debug ("picture %d: %s", picture_num, filename_ptr);
	}

	uri_start = uri_end + 1;
    }
    if (*uri_start != '\0')
    {
       	filename_ptr = g_filename_from_uri (uri_start, NULL, NULL);
	if (filename_ptr != NULL)
	{
	    g_ptr_array_add (picture_paths, filename_ptr);
	    picture_num ++;
	    g_debug ("picture %d: %s", picture_num, filename_ptr);
	}
    }
    //ensure we don't have a empty picture uris
    if (picture_num == 0)
    {
	g_ptr_array_add (picture_paths, g_strdup(BG_DEFAULT_PICTURE));
	picture_num =1;
    }
}
#endif

static void
register_account_service_background_path (GsdBackgroundManager* manager, const char* cur_pict)
{
    GError* error = NULL;

    if (manager->priv->accounts_proxy == NULL)
    {
	int flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES|
		    G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS;
	
        GDBusProxy* _proxy = NULL;
	_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
						flags,
						NULL,
						"org.freedesktop.Accounts",
						"/org/freedesktop/Accounts",
						"org.freedesktop.Accounts",
						NULL,
						&error);
	if (error != NULL)
	{
	    g_debug ("connect org.freedesktop.Accounts failed");
	    g_error_free (error);
	}

	gint64 user_id = 0;
	user_id = (gint64)geteuid ();
	//g_debug ("call FindUserById: uid = %i", user_id);

        GVariant* object_path_var = NULL;
	error = NULL;
	object_path_var = g_dbus_proxy_call_sync (_proxy, "FindUserById",
						  g_variant_new ("(x)", user_id),
						  G_DBUS_CALL_FLAGS_NONE,
						  -1,
						  NULL,
						  &error);
	if (error != NULL)
	{
	    g_debug ("FindUserById: %s", error->message);
	    g_error_free (error);
	}

	char* object_path = NULL;
	g_variant_get (object_path_var, "(o)", &object_path);
	g_debug ("object_path : %s", object_path);

	g_variant_unref (object_path_var);
	g_object_unref (_proxy);

	//yeah, setup another proxy to set background
	manager->priv->accounts_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
						       flags,
						       NULL,
						       "org.freedesktop.Accounts",
						       object_path,
						       "org.freedesktop.Accounts.User",
						       NULL,
						       &error);
	if (error != NULL)
	{
	    g_debug ("connect to %s failed", object_path);
	    g_error_free (error);
	}
	g_free (object_path);
    }
    
    error = NULL;
    g_dbus_proxy_call_sync (manager->priv->accounts_proxy, 
			    "SetBackgroundFile",
			    g_variant_new("(s)",cur_pict),
			    G_DBUS_CALL_FLAGS_NONE,
			    -1, 
			    NULL, 
			    &error);
    if (error != NULL)
    {
	g_debug ("org.freedesktop.Accounts.User: SetBackgroundFile %s failed", cur_pict);
	g_error_free (error);
    }
}

static void
bg_settings_cur_pict_changed (GSettings *settings, gchar *key, gpointer user_data)
{
    if (g_strcmp0 (key, BG_CURRENT_PICT))
	return;

    GsdBackgroundManager* manager = user_data;
    char* cur_pict_path = g_settings_get_string (settings, BG_CURRENT_PICT);
    if (update_prev_pict_path (cur_pict_path))
    {
	register_account_service_background_path (manager, cur_pict_path);
	start_gaussian_helper (cur_pict_path);
    }
}

static void
gnome_bg_settings_cur_pict_changed (GSettings *settings, gchar *key, gpointer user_data)
{
    if (g_strcmp0 (key, GNOME_BG_PICTURE_URI))
	return;

    GsdBackgroundManager* manager = user_data;

    char* pict_uri = g_settings_get_string (settings, GNOME_BG_PICTURE_URI);
    g_settings_set_string (manager->priv->deepin_settings, BG_PICTURE_URIS, pict_uri);
    g_free (pict_uri);
}

static void
initial_setup (GsdBackgroundManager* manager)
{
    prev_pict_path = NULL;

    char* cur_pict_path = NULL; 
    cur_pict_path = g_settings_get_string (manager->priv->deepin_settings, BG_CURRENT_PICT);

    register_account_service_background_path (manager, cur_pict_path);

    start_gaussian_helper (cur_pict_path);

    g_free (cur_pict_path);
}

DEEPIN_EXPORT void
bg_util_init (GsdBackgroundManager* manager)
{
    //work_list = g_ptr_array_new ();
    //src_uri_ht = g_hash_table_new_full (g_str_hash, g_str_equal,
    //					g_free, NULL);

    manager->priv->accounts_proxy = NULL;

    manager->priv->deepin_settings = g_settings_new (BG_SCHEMA_ID);
    manager->priv->gnome_settings = g_settings_new (GNOME_BG_SCHEMA_ID);

    //serialize access to current picture.
    g_signal_connect (manager->priv->deepin_settings, "changed::current-picture",
		      G_CALLBACK (bg_settings_cur_pict_changed), manager);
    g_signal_connect (manager->priv->gnome_settings, "changed::picture-uri",
		      G_CALLBACK (gnome_bg_settings_cur_pict_changed), manager);

    initial_setup (manager);
}
