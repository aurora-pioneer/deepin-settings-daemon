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

#include <gio/gio.h>

static GSettings *m_xrandr_settings = NULL;

int deepin_media_key_init() 
{
    m_xrandr_settings = g_settings_new("org.gnome.settings-daemon.plugins.xrandr");

    return 0;
}

void deepin_media_key_cleanup() 
{
    if (m_xrandr_settings) {
        g_object_unref(m_xrandr_settings);
        m_xrandr_settings = NULL;
    }
}

int deepin_media_key_set_brightness(double brightness) 
{
    g_settings_set_double(m_xrandr_settings, "brightness", brightness);
    g_settings_sync();
}
