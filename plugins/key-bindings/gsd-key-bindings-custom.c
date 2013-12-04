/**
 * Copyright (c) 2011 ~ 2013 Deepin, Inc.
 *               2011 ~ 2013 jouyouyun
 *
 * Author:      jouyouyun <jouyouwen717@gmail.com>
 * Maintainer:  jouyouyun <jouyouwen717@gmail.com>
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

#include "keybinder.h"
#include "gsd-keygrab.h"
#include "gsd-key-bindings-custom.h"

#define KEY_BINDING_ID	"com.deepin.daemon.key-binding"
#define KEY_BINDING_ADD_ID	"com.deepin.daemon.key-binding.custom"
#define KEY_BINDING_ADD_PATH	"/com/deepin/daemon/key-binding/profiles/"

#define KEY_COUNT	"count"
#define KEY_ID	"id"
#define KEY_NAME	"name"
#define KEY_SHORTCUT	"shortcut"
#define KEY_ACTION	"action"
#define KEY_COUNT_BASE	1000

static void on_settings_changed_cb (GSettings *settings, gchar *key,
                                    gpointer user_data);
static void listen_settings_changed (const char *id);
static void listen_settings_changed_all (const gint count);
static gboolean grab_custom_key (const char *shortcut);
static void grab_custom_key_full (void);
static gboolean ungrab_custom_key (const char *shortcut);
static void ungrab_custom_key_full (void);
static CustomGrabKey *new_custom_grab_entry (gchar *shortcut, gchar *action);
static void remove_grab_entry (CustomGrabKey *entry);

GHashTable *custom_table;
extern GSList *screens;

void
init_custom_settings (void)
{
    custom_table = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                          NULL,
                                          (GDestroyNotify)remove_grab_entry);
    GSettings *gs = g_settings_new (KEY_BINDING_ID);
    gint count = g_settings_get_int (gs, KEY_COUNT);

    listen_settings_changed_all (count);
    g_signal_connect (gs, "changed",
                      G_CALLBACK (on_settings_changed_cb), NULL);
}

void
finalize_custom_settings (void)
{
    ungrab_custom_key_full ();
    g_hash_table_remove_all (custom_table);
    g_hash_table_destroy (custom_table);
}

static void
listen_settings_changed_all (const gint count)
{
    g_return_if_fail (count > 0);

    gint i = 0;

    for ( ; i < count; i++ ) {
        gchar *id = g_strdup_printf ("%d", KEY_COUNT_BASE + i);
        listen_settings_changed (id);
        g_free (id);
    }
}

static void
listen_settings_changed (const gchar *id)
{
    g_return_if_fail (id != NULL);

    gchar *path = g_strdup_printf ("%s%s/", KEY_BINDING_ADD_PATH, id);
    GSettings *gs = g_settings_new_with_path (KEY_BINDING_ADD_ID,
                    path);
    g_free (path);

    gint key_id = g_settings_get_int (gs, KEY_ID);
    gchar *shortcut = g_settings_get_string (gs, KEY_SHORTCUT);
    gchar *action = g_settings_get_string (gs, KEY_ACTION);

    if ( grab_custom_key (shortcut) ) {
        CustomGrabKey *entry = new_custom_grab_entry (shortcut, action);
        g_hash_table_insert (custom_table, GINT_TO_POINTER(key_id), entry);
    }

    g_signal_connect (gs, "changed",
                      G_CALLBACK(on_settings_changed_cb), NULL);
}

static void
on_settings_changed_cb (GSettings *settings, gchar *key,
                        gpointer user_data)
{
    if ( g_strcmp0(key, KEY_COUNT) == 0 ) {
        ungrab_custom_key_full ();
        g_hash_table_remove_all (custom_table);
        gint count = g_settings_get_int (settings, KEY_COUNT);
        listen_settings_changed_all (count);
    }

    if ( g_strcmp0 (key, KEY_NAME) == 0 ) {
        return ;
    }

    if ( g_strcmp0 (key, KEY_SHORTCUT) == 0 ) {
        ungrab_custom_key_full ();
        gint key_id = g_settings_get_int (settings, KEY_ID);
        gchar *shortcut = g_settings_get_string (settings, KEY_SHORTCUT);
        CustomGrabKey *entry = g_hash_table_lookup (custom_table,
                               GINT_TO_POINTER (key_id));
        g_free (entry->shortcut);
        entry->shortcut = shortcut;
        grab_custom_key_full ();
        return ;
    }

    if ( g_strcmp0 (key, KEY_ACTION) == 0 ) {
        gint key_id = g_settings_get_int (settings, KEY_ID);
        gchar *action = g_settings_get_string (settings, KEY_ACTION);
        CustomGrabKey *entry = g_hash_table_lookup (custom_table,
                               GINT_TO_POINTER (key_id));
        g_free (entry->action);
        entry->action = action;
        return ;
    }
}

static gboolean
grab_custom_key (const char *shortcut)
{
    g_return_val_if_fail ( shortcut != NULL, FALSE );

    Key *key = parse_key (shortcut);

    if ( key == NULL ) {
        return FALSE;
    }

    grab_key_unsafe (key, TRUE, GSD_KEYGRAB_NORMAL, screens);
    free_key (key);

    return TRUE;
}

static gboolean
ungrab_custom_key (const char *shortcut)
{
    g_return_val_if_fail ( shortcut != NULL, FALSE );

    Key *key = parse_key (shortcut);

    if ( key == NULL ) {
        return FALSE;
    }

    grab_key_unsafe (key, FALSE, GSD_KEYGRAB_NORMAL, screens);
    free_key (key);

    return TRUE;
}

static void
grab_custom_key_full (void)
{
    GHashTableIter iter;
    CustomGrabKey *entry;

    g_hash_table_iter_init (&iter, custom_table);

    while ( g_hash_table_iter_next(&iter, NULL, (void **)&entry) ) {
        grab_custom_key (entry->shortcut);
    }
}

static void
ungrab_custom_key_full (void)
{
    GHashTableIter iter;
    CustomGrabKey *entry;

    g_hash_table_iter_init (&iter, custom_table);

    while ( g_hash_table_iter_next(&iter, NULL, (void **)&entry) ) {
        ungrab_custom_key (entry->shortcut);
    }
}

static CustomGrabKey *
new_custom_grab_entry (gchar *shortcut, gchar *action)
{
    g_return_val_if_fail ( (shortcut != NULL) && (action != NULL), NULL );

    CustomGrabKey *entry = g_new0 (CustomGrabKey, 1);
    entry->shortcut = shortcut;
    entry->action = action;

    return entry;
}

static void
remove_grab_entry (CustomGrabKey *entry)
{
    g_return_if_fail (entry != NULL);

    g_free (entry->shortcut);
    g_free (entry->action);
    g_free (entry);
}
