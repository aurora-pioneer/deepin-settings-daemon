/* 
 * Copyright (C) 2012 Deepin, Inc.
 *               2012 Zhai Xiang
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

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <gio/gio.h>

#include "gsd-xrandr-manager.h"

typedef struct _output output_t;

static Display *m_dpy = NULL;
static Window m_root = -1;
static output_t *m_outputs = NULL;
static XRRScreenResources *m_res = NULL;

static void m_get_screen(int current);
static void m_brightness_changed(GSettings *settings, gchar *key, gpointer user_data);

static void m_get_screen(int current) 
{

}

static void m_brightness_changed(GSettings *settings, gchar *key, gpointer user_data) 
{
    double value = g_settings_get_double(settings, key);

}

int deepin_xrandr_init(GsdXrandrManager *manager) 
{
    if (!manager) 
        return -1;

    m_dpy = XOpenDisplay(NULL);
    if (!m_dpy) 
        return -1;
   
    m_root = RootWindow(m_dpy, DefaultScreen(m_dpy));

    manager->priv->settings = g_settings_new(CONF_SCHEMA);

    g_signal_connect(manager->priv->settings, "changed::brightness", 
                     G_CALLBACK(m_brightness_changed), NULL);
    
    return 0;
}
