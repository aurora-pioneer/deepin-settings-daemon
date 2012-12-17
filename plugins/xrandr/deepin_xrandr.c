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
#include <limits.h>
#include <pwd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "gsd-xrandr-manager.h"
#include "xrandr.h"

#define BUF_SIZE 1024
#define XRANDR_PROG_NAME "Deepin XRandR"

static GFile *m_config_file = NULL;
static GFileMonitor *m_config_file_monitor = NULL;

static void m_config_file_changed(GFileMonitor *monitor, 
                                  GFile *file, 
                                  GFile *other_file, 
                                  GFileMonitorEvent event_type, 
                                  gpointer user_data);
static void m_changed_brightness(GSettings *settings, gchar* key, gpointer user_data);
static void m_set_brightness(GSettings *settings, double value);
static void m_init_brightness(GSettings *settings);
static void m_init_screen_size(GSettings *settings);

static void m_config_file_changed(GFileMonitor *monitor, 
                                  GFile *file, 
                                  GFile *other_file, 
                                  GFileMonitorEvent event_type, 
                                  gpointer user_data) 
{
    if (G_FILE_MONITOR_EVENT_CHANGED != event_type) 
        return;

    printf("DEBUG m_config_file_changed %d\n", event_type);
}

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

void deepin_xrandr_set_output_names(GSettings *settings) 
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

int deepin_xrandr_init(GSettings *settings) 
{
    char backup_filename[PATH_MAX] = {'\0'}; 
    struct passwd *pw = NULL;

    g_signal_connect(settings, "changed::brightness", m_changed_brightness, NULL);
    
    deepin_xrandr_set_output_names(settings);
    m_init_brightness(settings);

    pw = getpwuid(getuid());
    if (!pw) {
        return -1;
    }
    sprintf(backup_filename, "%s/.config/monitors.xml", pw->pw_dir);
    m_config_file = g_file_new_for_path(backup_filename);
    if (!m_config_file) 
        return -1;

    m_config_file_monitor = g_file_monitor_file(m_config_file, G_FILE_MONITOR_NONE, NULL, NULL);
    if (!m_config_file_monitor) 
        return -1;

    g_signal_connect(m_config_file_monitor, "changed", m_config_file_changed, NULL);

    return 0;
}

void deepin_xrandr_cleanup() 
{
    xmlCleanupParser();
    
    if (m_config_file_monitor) {
        g_object_unref(m_config_file_monitor);
        m_config_file_monitor = NULL;
    }
    
    if (m_config_file) {
        g_object_unref(m_config_file);
        m_config_file = NULL;
    }

    xrandr_cleanup();
}
