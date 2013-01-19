/* 
 * Copyright (C) 2013 Deepin, Inc.
 *               2013 Zhai Xiang
 *
 * Author:     Zhai Xiang <zhaixiang@linuxdeepin.com>
 * Maintainer: Zhai Xiang <zhaixiang@linuxdeepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <dbus/dbus-glib.h>

#define APP_ID "deepin-settings-daemon"

typedef enum {                                                                  
    GSM_INHIBITOR_FLAG_LOGOUT      = 1,                                
    GSM_INHIBITOR_FLAG_SWITCH_USER = 2,                                
    GSM_INHIBITOR_FLAG_SUSPEND     = 4,
    GSM_INHIBITOR_FLAG_IDLE        = 8,
} GsmInhibitFlag;                                                 

static GDBusProxy *m_session_proxy = NULL;
static GSettings *m_settings = NULL;
static GtkWidget *m_window = NULL;
static guint m_cookie = 0;

static void m_on_setup_inhibit(GtkWidget *widget, gpointer user_data);

static void m_on_setup_inhibit(GtkWidget *widget, gpointer user_data) 
{
    GVariant *retval = NULL;
    GError *error = NULL;
    const char *reason = "Deepin Settings Daemon is in progress.";                                                     
    guint toplevel_xid = 0;                            
    guint flags = 0;

    toplevel_xid = gdk_x11_window_get_xid(gtk_widget_get_window(m_window));
    if (!toplevel_xid) 
        return;

    /* TODO: HACKER it is VIRUS!!!
    flags = GSM_INHIBITOR_FLAG_LOGOUT | 
            GSM_INHIBITOR_FLAG_SWITCH_USER | 
            GSM_INHIBITOR_FLAG_SUSPEND | 
            GSM_INHIBITOR_FLAG_IDLE;
    */

    flags = GSM_INHIBITOR_FLAG_IDLE;

    retval = g_dbus_proxy_call_sync(m_session_proxy, 
                                    "Inhibit", 
                                    g_variant_new("(susu)", 
                                                  APP_ID, 
                                                  toplevel_xid, 
                                                  reason, 
                                                  flags), 
                                    G_DBUS_CALL_FLAGS_NONE, 
                                    -1, 
                                    NULL, 
                                    &error);

    if (!retval) {                                                   
        g_warning("Inhibited failed: %s", error->message);           
        g_error_free(error);                                           
        return;                                                   
    }

    g_variant_get(retval, "(u)", &m_cookie);                                    
    g_variant_unref(retval);
}

static void m_do_uninhibit() 
{
    g_dbus_proxy_call(m_session_proxy,                                      
                      "Uninhibit",                                   
                      g_variant_new("(u)", m_cookie),                                        
                      G_DBUS_CALL_FLAGS_NONE, 
                      -1, 
                      NULL, 
                      NULL, 
                      NULL);                               
                                                                                
    m_cookie = 0;           
}

void deepin_power_init(GDBusProxy *session_proxy, GSettings *settings) 
{
    if (!session_proxy) 
        return;

    if (!settings) 
        return;

    m_session_proxy = session_proxy;
    m_settings = settings;
    
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_opacity(m_window, 0.0);
    gtk_window_set_default_size(m_window, 0, 0);
    gtk_window_set_skip_taskbar_hint(m_window, FALSE);
    g_signal_connect(m_window, "show", G_CALLBACK(m_on_setup_inhibit), NULL);
    gtk_widget_show(m_window);
    gtk_widget_set_visible(m_window, FALSE);
}

void deepin_power_cleanup() 
{
    m_do_uninhibit();

    if (m_window) {
        g_object_unref(m_window);
        m_window = NULL;
    }
}
