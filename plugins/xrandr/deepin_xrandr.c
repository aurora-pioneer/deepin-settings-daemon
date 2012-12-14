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

#include <gio/gio.h>

#include "gsd-xrandr-manager.h"
#include "xrandr.h"

#define BUF_SIZE 1024
#define XRANDR_PROG_NAME "Deepin XRandR"

static void m_changed_brightness(GSettings *settings, gchar* key, gpointer user_data);
static void m_init_output_names(GSettings *settings);
static void m_set_brightness(GSettings *settings, double value);
static void m_init_brightness(GSettings *settings);
static void m_init_screen_size(GSettings *settings);

static void m_changed_brightness(GSettings *settings, gchar *key, gpointer user_data) 
{
    double value = g_settings_get_double(settings, "brightness");

    m_set_brightness(settings, value);
}

static void m_set_brightness(GSettings *settings, double value) 
{
    char **output_names = g_settings_get_strv(settings, "output-names");
    char value_str[BUF_SIZE];
    int i = 0;

    if (!output_names) 
        return;
    
    while (output_names[i]) {
        /* TODO: filter the disconnected output */
        if (strcmp(output_names[i], "NULL") == 0) { 
            i++;
            continue;
        }

        memset(value_str, 0, BUF_SIZE);
        sprintf(value_str, "%f", value);
        char *argv[] = {XRANDR_PROG_NAME, 
                        "--output", 
                        output_names[i], 
                        "--brightness", 
                        value_str};
        xrandr_main(5, argv);
        xrandr_cleanup();
        
        i++;
    }
}

static void m_init_output_names(GSettings *settings) 
{
    char *argv[] = {XRANDR_PROG_NAME};

    xrandr_main(1, argv);
    g_settings_set_strv(settings, "output-names", xrandr_get_output_names());
    xrandr_cleanup();
}

static void m_init_brightness(GSettings *settings) 
{
    double value = g_settings_get_double(settings, "brightness");
    
    if (value <= 0.0 || value > 1.0) 
        return;

    m_set_brightness(settings, value);
}

int deepin_xrandr_init(GsdXrandrManager *manager) 
{
    if (!manager) 
        return -1;

    manager->priv->settings = g_settings_new(CONF_SCHEMA);
    g_signal_connect(manager->priv->settings, 
                     "changed::brightness", 
                     m_changed_brightness, 
                     NULL);
    
    m_init_output_names(manager->priv->settings);

    m_init_brightness(manager->priv->settings);
    
    return 0;
}

void deepin_xrandr_cleanup() 
{
    xrandr_cleanup();
}
