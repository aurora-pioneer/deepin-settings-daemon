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

#include "gsd-xrandr-manager.h"

#include <gio/gio.h>

static void m_brightness_changed(GSettings *settings, gchar *key, gpointer user_data);

static void m_brightness_changed(GSettings *settings, gchar *key, gpointer user_data) 
{
    double brightness = g_settings_get_double(settings, key);

    printf("DEBUG m_brightness_changed %s %d\n", key, brightness);
}

int deepin_xrandr_init(GsdXrandrManager *manager) 
{
    if (!manager) 
        return -1;
    
    manager->priv->settings = g_settings_new(CONF_SCHEMA);

    g_signal_connect(manager->priv->settings, "changed::brightness", 
                     G_CALLBACK(m_brightness_changed), NULL);
    
    return 0;
}
