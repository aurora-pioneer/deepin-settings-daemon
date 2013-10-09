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

#include "parse-super.h"

void xrecord_thread ();
void handle_key (XCape_t* self, int key_event);

extern GHashTable* key_table;
static gint key_press_cnt = 0;
static XCape_t* self_xcape;

gboolean init_xrecord ()
{
    GThread* xrcd_thrd;

    xrcd_thrd = g_thread_new ("init XRecord", 
            (GThreadFunc)xrecord_thread, NULL);

    if ( !xrcd_thrd ) {
        return FALSE;
    }

    return TRUE;
}

void xrecord_thread ()
{
    int dummy;
    
    self_xcape = calloc (sizeof (XCape_t), 1);
    if ( !self_xcape ) {
        g_debug ("Got XCape memory failed!\n");
        /*return FALSE;*/
        g_thread_exit (NULL);
    }

    self_xcape->data_conn = XOpenDisplay (NULL);
    self_xcape->ctrl_conn = XOpenDisplay (NULL);

    if ( !self_xcape->data_conn || !self_xcape->ctrl_conn ) {
        g_debug ("Unable to connect to X11 display!\n");
        /*return FALSE;*/
        g_thread_exit (NULL);
    }

    if ( !XQueryExtension (self_xcape->ctrl_conn, 
                "XTEST", &dummy, &dummy, &dummy) ) {
        g_debug ("Xtst extension missing!\n");
        /*return FALSE;*/
        g_thread_exit (NULL);
    }

    if ( !XRecordQueryVersion (self_xcape->ctrl_conn, &dummy, &dummy) ) {
        g_debug ("Failed to obtain xrecord version!\n");
        /*return FALSE;*/
        g_thread_exit (NULL);
    }

    if ( !XkbQueryExtension (self_xcape->ctrl_conn, &dummy, &dummy, 
                &dummy, &dummy, &dummy)) {
        g_debug ("Failed to obtain xkb version!\n");
        /*return FALSE;*/
        g_thread_exit (NULL);
    }

    XRecordRange* rec_range = XRecordAllocRange ();
    rec_range->device_events.first = KeyPress;
    rec_range->device_events.last = ButtonRelease;
    XRecordClientSpec client_spec = XRecordAllClients;

    self_xcape->record_ctx = XRecordCreateContext (self_xcape->ctrl_conn, 
            0, &client_spec, 1, &rec_range, 1);
    if ( self_xcape->record_ctx == 0 ) {
        g_debug ("Failed to create xrecord context!\n");
        /*return FALSE;*/
        g_thread_exit (NULL);
    }

    /*XSync (self_xcape->ctrl_conn, FALSE);*/
    XFlush (self_xcape->ctrl_conn);

    if ( !XRecordEnableContext (self_xcape->data_conn, 
                self_xcape->record_ctx, intercept, (XPointer)self_xcape) ) {
        g_debug ("Failed to enable xrecord context!\n");
        /*return FALSE;*/
        g_thread_exit (NULL);
    }

    g_thread_exit (NULL);
}

void finalize_xrecord ()
{
    if ( !XRecordFreeContext (self_xcape->ctrl_conn, 
                self_xcape->record_ctx) ) {
        g_debug ("Failed to free xrecord connect!\n");
    }

    XCloseDisplay (self_xcape->ctrl_conn);
    XCloseDisplay (self_xcape->data_conn);
    g_hash_table_destroy (key_table);
    g_debug ("init xcape exiting!\n");

    return ;
}

void
handle_key (XCape_t* self, int key_event)
{
    if ( key_event == KeyPress ) {
        g_debug ("Key Pressed!\n");
        key_press_cnt++;
    } else {
        g_debug ("Key Released!\n");
        if ( key_press_cnt == 1 ) {
            if ( is_grabbed () ) {
               return ; 
            }
            KeySym key_sym;

            key_sym = XkbKeycodeToKeysym (self->ctrl_conn, self->key, 0, 0);
            gchar* key_str = g_strdup_printf ("%lu", (gulong)key_sym);
            gchar* cmd = g_hash_table_lookup (key_table, key_str);
            if ( cmd ) {
                if ( system (cmd) == -1 ) {
                    g_debug ("system exec error!");
                }
            }
            g_free ( key_str);
        }
        key_press_cnt = 0;
    }
}

void
intercept (XPointer user_data, XRecordInterceptData* data)
{
    XCape_t *self = (XCape_t*)user_data;

    if (data->category == XRecordFromServer)
    {
        int     key_event = data->data[0];
        KeyCode key_code  = data->data[1];
        g_debug ("Intercepted key event %d, key code %d\n",
                key_event, key_code);
        if ( key_code == 133 ) {
        }
        self->key = key_code;

        if (key_event == ButtonPress) {
            g_debug ("***Button Pressed!\n");
        } else if (key_event == ButtonRelease) {
            g_debug ("***Mouse Pressed!\n");
        } else {
            g_debug ("***Other Pressed!\n");
            handle_key (self, key_event);
        }
    }
}

void insert_table_record (KeySym keysym, gchar* data)
{
    gchar* keysym_str = g_strdup_printf ("%lu", (gulong)keysym);

    g_hash_table_insert (key_table, keysym_str, g_strdup (data));
}

void remove_table_all_record ()
{
    g_hash_table_remove_all (key_table);
}

void remove_table_record (KeySym keysym)
{
    gchar* keysym_str = g_strdup_printf ("%lu", (gulong)keysym);

    g_hash_table_remove (key_table, keysym_str);
}

gboolean is_grabbed ()
{
    Display* dpy;
    Window root;
    int ret;

    dpy = XOpenDisplay (0);
    root = DefaultRootWindow (dpy);

    ret = XGrabKeyboard (dpy, root, False, 
            GrabModeSync, GrabModeSync, CurrentTime);
    if ( ret == AlreadyGrabbed ) {
        g_print ("AlreadyGrabbed!\n");
        XCloseDisplay(dpy);
        return True;
    }

    XCloseDisplay(dpy);
    return False;
}
