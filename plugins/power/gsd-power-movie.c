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

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <string.h>

#include "gsd-power-movie.h"

static void on_active_window_changed (WnckScreen* screen, 
        WnckWindow* pre_active_window, gpointer data);
static void on_state_changed (WnckWindow* window, 
        WnckWindowState changed_mask, WnckWindowState new_state, 
        gpointer data);

static int active_set_power = 0;
static int has_set_flag = 0;
static gchar* user_plan = NULL;

static gulong active_sig_id = 0;
static gulong state_sig_id = 0;

void init_power_movie (void)
{
    gdk_init (NULL, NULL);
    WnckScreen* screen = wnck_screen_get_default ();

    active_sig_id = g_signal_connect (screen, "active-window-changed", 
            G_CALLBACK (on_active_window_changed), NULL);
}

void finalize_reset_power ()
{
    WnckScreen* screen = wnck_screen_get_default ();
    WnckWindow* active_window = wnck_screen_get_active_window (screen);

    if ( active_sig_id ) {
        g_signal_handler_disconnect (screen, active_sig_id);
    }

    if ( state_sig_id ) {
        g_signal_handler_disconnect (active_window, state_sig_id);
    }
    
    if ( user_plan ) {
        g_free (user_plan);
        user_plan = NULL;
    }
}

static void on_active_window_changed (WnckScreen* screen, 
        WnckWindow* pre_active_window, gpointer data)
{
    gulong pid = 0;
    gboolean is_change = FALSE;

    wnck_screen_force_update (screen);
    WnckWindow* active_window = wnck_screen_get_active_window (screen);

    if ( !active_window ) {
        g_warning ("No Active Window!\n");
        return ;
    }

    if ( state_sig_id ) {
        g_signal_handler_disconnect (pre_active_window, state_sig_id);
    }
    state_sig_id = g_signal_connect (active_window, "state-changed", 
            G_CALLBACK (on_state_changed), NULL);

    if ( wnck_window_is_fullscreen (active_window) ) {
        pid = wnck_window_get_pid (active_window);
        g_debug ("current pid: %lu\n", pid);

        is_change = power_setting_is_change (pid);
        if ( is_change ) {
            active_set_power = 1;
            if ( user_plan ) {
                g_free (user_plan);
                user_plan = NULL;
            }
            user_plan = get_user_power_plan ();

            g_debug ("Set power plan for high performance!\n\n");
            set_power_plan (HIGH_PERFORMANCER);
        }
    } else {
        if ( active_set_power ) {
            active_set_power = 0;
            g_debug ("**pre state is full screen###!\n");
            set_power_plan (user_plan);
        }
    }
}

static void on_state_changed (WnckWindow* window, 
        WnckWindowState changed_mask, WnckWindowState new_state, 
        gpointer data)
{
    gulong pid = 0;
    gboolean is_change = FALSE;

    if ( changed_mask == WNCK_WINDOW_STATE_FULLSCREEN ) {
        pid = wnck_window_get_pid (window);
        g_debug ("current pid: %lu\n", pid);

        is_change = power_setting_is_change (pid);
        if ( is_change ) {
            if ( !has_set_flag ) {
                has_set_flag = 1;
                if ( user_plan ) {
                    g_free (user_plan);
                    user_plan = NULL;
                }
                user_plan = get_user_power_plan ();

                g_debug ("Set power plan for high performance!\n");
                set_power_plan (HIGH_PERFORMANCER);
            } else {
                has_set_flag = 0;
                g_debug ("**pre state is full screen###!\n");
                set_power_plan (user_plan);
            }
        }
    }

    return ;
}

gchar* get_user_power_plan ()
{
    gchar* cur_plan = NULL;
    GSettings* power_settings = NULL;

    power_settings = g_settings_new ("org.gnome.settings-daemon.plugins.power");
    cur_plan = g_settings_get_string (power_settings, "current-plan");
    g_debug ("current plan: %s\n", cur_plan);

    g_object_unref (power_settings);

    return cur_plan;
}

int set_power_plan (const gchar* plan)
{
    gboolean is_ok;
    GSettings* power_settings = NULL;

    if ( !plan ) {
        g_warning ("args error in set power plan!\n");
        return -1;
    }

    power_settings = g_settings_new ("org.gnome.settings-daemon.plugins.power");
    is_ok = g_settings_set_string (power_settings, "current-plan", plan);
    if ( !is_ok ) {
        g_warning ("Set power plan failed: %s\n", plan);
        return -1;
    }
    g_settings_sync ();
    g_object_unref (power_settings);

    return 0;
}

gboolean power_setting_is_change (gulong pid)
{
    int i = 0;
    int exist_flag = 0;
    gchar* file_path = NULL;
    gchar* content = NULL;
    gsize length;
    GError* error = NULL;
    gboolean is_ok;

    file_path = g_strdup_printf ("/proc/%lu/cmdline", pid);
    is_ok = g_file_get_contents (file_path, &content, &length, &error);
    g_free (file_path);
    if ( !is_ok ) {
        g_warning ("Get file content failed: %s\n", error->message);
        g_error_free (error);
        return FALSE;
    }
    
    g_debug ("size: %d\n", length);
    for (; i < length; i++ ) {
        if ( memcmp("libflash", (content + i), 8) == 0 ) {
            g_debug ("flash exist!\n");
            exist_flag = 1;
            break;
        } else if ( memcmp("chrome", (content + i), 6) == 0 ) {
            g_debug ("chrome exist!\n");
            exist_flag = 1;
            break;
        } else if ( memcmp("operaplugin", (content + i), 11) == 0 ) {
            g_debug ("opera plugin exist!\n");
            exist_flag = 1;
            break;
        } else if ( memcmp("mplayer", (content + i), 7) == 0 ) {
            g_debug ("mplayer exist!\n");
            exist_flag = 1;
            break;
        }
    }
    g_free (content);
    
    if ( !exist_flag ) {
        g_warning ("current app not in lists of allow!\n");
        return FALSE;
    }

    return TRUE;
}
