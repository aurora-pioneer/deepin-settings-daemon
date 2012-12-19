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

#define BUF_SIZE 10

static GFile *m_config_file = NULL;
static GFileMonitor *m_config_file_monitor = NULL;

static void m_config_file_changed(GFileMonitor *monitor, 
                                  GFile *file, 
                                  GFile *other_file, 
                                  GFileMonitorEvent event_type, 
                                  gpointer user_data);
static void m_screen_changed(GnomeRRScreen *screen, gpointer user_data);
static void m_set_output_names(GnomeRRScreen *screen, GSettings *settings);
static void m_changed_brightness(GSettings *settings, gchar* key, gpointer user_data);
static void m_set_brightness(GnomeRRScreen *screen, GSettings *settings);

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

static void m_screen_changed(GnomeRRScreen *screen, gpointer user_data) 
{
    GSettings *settings = (GSettings *) user_data;

    m_set_output_names(screen, settings);
}

static void m_changed_brightness(GSettings *settings, gchar *key, gpointer user_data) 
{
    GnomeRRScreen *screen = (GnomeRRScreen *) user_data;
    
    m_set_brightness(screen, settings);
}

static void m_set_brightness(GnomeRRScreen *screen, GSettings *settings) 
{
    char **output_names = NULL;
    double value = 0.0;
    char value_str[BUF_SIZE];
    int i = 0;
    
    output_names = g_settings_get_strv(settings, "output-names");
    value = g_settings_get_double(settings, "brightness");

    if (!output_names) 
        return;

    if (value <= 0.0 || value > 1.0) 
        return;
    
    while (output_names[i]) {
        if (strcmp(output_names[i], "NULL") == 0) {
            i++;
            continue;
        }
        
        memset(value_str, 0, BUF_SIZE);
        sprintf(value_str, "%f", value);
        char *argv[] = {"Deepin XRandR", 
                        "--output", 
                        output_names[i], 
                        "--brightness", 
                        value_str};
        xrandr_main(5, argv);
        xrandr_cleanup();
        
        i++;
    }

    if (output_names) {
        g_strfree(output_names);
        output_names = NULL;
    }
}

static void m_set_output_names(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRROutput **outputs = NULL;
    GnomeRROutput *output = NULL;
    char *output_name = NULL;
    gchar **strv = NULL;
    int i = 0;
    int count = 0;

    outputs = gnome_rr_screen_list_outputs(screen);
    if (!outputs) 
        return;

    while (outputs[count]) 
        count++;

    if (!count) 
        return;

    strv = malloc((count + 1) * sizeof(gchar *));
    if (!strv) 
        return;

    memset(strv, 0, (count + 1) * sizeof(gchar *));
    i = 0;
    while (outputs[i]) {
        if (gnome_rr_output_is_connected(outputs[i])) 
            output_name = gnome_rr_output_get_name(outputs[i]);
        else 
            output_name = "NULL";

        strv[i] = malloc(strlen(output_name) * sizeof(gchar));
        if (!strv[i]) {
            i++;
            continue;
        }

        memset(strv[i], 0, strlen(output_name) * sizeof(gchar));
        strcpy(strv[i], output_name);
        
        i++;
    }
    strv[count] = NULL;
   
    g_settings_set_strv(settings, "output-names", strv);
    g_settings_sync();

    if (strv) {
        i = 0;
        
        while (strv[i]) {
            if (strv[i]) { 
                free(strv[i]);
                strv[i] = NULL;
            }

            i++;
        }
        
        free(strv);
        strv = NULL;
    }
}

int deepin_xrandr_init(GnomeRRScreen *screen, GSettings *settings) 
{
    char backup_filename[PATH_MAX] = {'\0'}; 
    struct passwd *pw = NULL;

    /* TODO: GnomeRRScreen changed event */
    g_signal_connect(screen, "changed", m_screen_changed, settings);
    
    /* TODO: GSettings changed brightness key event */
    g_signal_connect(settings, "changed::brightness", m_changed_brightness, screen);
    
    m_set_output_names(screen, settings);
    m_set_brightness(screen, settings);

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

    /* TODO: GFile changed event */
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
}
