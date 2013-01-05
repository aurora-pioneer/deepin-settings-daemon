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
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "gsd-xrandr-manager.h"
#include "xrandr.h"

#define BUF_SIZE 1024

static GFile *m_config_file = NULL;
static GFileMonitor *m_config_file_monitor = NULL;

static void m_config_file_changed(GFileMonitor *monitor, 
                                  GFile *file, 
                                  GFile *other_file, 
                                  GFileMonitorEvent event_type, 
                                  gpointer user_data);
static void m_screen_changed(GnomeRRScreen *screen, gpointer user_data);
static void m_set_output_names(GnomeRRScreen *screen, GSettings *settings);
static void m_set_multi_monitors(GnomeRRScreen *screen, GSettings *settings);
static void m_changed_brightness(GSettings *settings, gchar *key, 
                                 gpointer user_data);
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
    m_set_multi_monitors(screen, settings);
}

static void m_changed_brightness(GSettings *settings, gchar *key, 
                                 gpointer user_data) 
{
    GnomeRRScreen *screen = (GnomeRRScreen *) user_data;
    
    m_set_brightness(screen, settings);
}

static void m_set_brightness(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRRConfig *config = NULL;
    GnomeRROutputInfo **output_infos = NULL;
    GnomeRROutput *output = NULL;
    char *output_name = NULL;
    double value = 0.0;
    char value_str[BUF_SIZE];
    int i = 0;
    
    config = gnome_rr_config_new_current(screen, NULL);
    if (!config) 
        return;

    output_infos = gnome_rr_config_get_outputs(config);
    if (!output_infos) 
        return;

    value = g_settings_get_double(settings, "brightness");
    if (value <= 0.0 || value > 1.0) 
        return;

    while (output_infos[i]) {
        /* Check whether the output is primary or not at first */
        if (!gnome_rr_output_info_get_primary(output_infos[i])) {
            i++;
            continue;
        }

        output_name = gnome_rr_output_info_get_name(output_infos[i]);
        if (!output_name) {
            i++;
            continue;
        }

        output = gnome_rr_screen_get_output_by_name(screen, output_name);
        if (!output) {
            i++;
            continue;
        }

        if (!gnome_rr_output_is_connected(output)) {
            i++;
            continue;
        }

        memset(value_str, 0, BUF_SIZE);
        sprintf(value_str, "%f", value);
        char *argv[] = {"Deepin XRandR", 
                        "--output", 
                        output_name, 
                        "--brightness", 
                        value_str};
        xrandr_main(5, argv);
        xrandr_cleanup();
        
        i++;
    }

    if (config) {
        g_object_unref(config);
        config = NULL;
    }
}

static void m_set_multi_monitors(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRRConfig *config = NULL;                                               
    GnomeRROutputInfo **output_infos = NULL;                                    
    GnomeRROutput **outputs = NULL;
    char *primary_output_name = NULL;
    char *other_output_name = NULL;
    int i = 0;
                                                                                
    config = gnome_rr_config_new_current(screen, NULL);                         
    if (!config)                                                                
        return;                                                                 
                                                                                
    output_infos = gnome_rr_config_get_outputs(config);                         
    if (!output_infos)                                                          
        return;

    outputs = gnome_rr_screen_list_outputs(screen);
    if (!outputs) 
        return;

    while (outputs[i] && output_infos[i]) {
        if (!gnome_rr_output_is_connected(outputs[i])) {
            i++;
            continue;
        }
        
        if (gnome_rr_output_info_get_primary(output_infos[i])) {
            primary_output_name = gnome_rr_output_info_get_name(output_infos[i]);
            i++;
            continue;
        }

        other_output_name = gnome_rr_output_info_get_name(output_infos[i]);
        
        i++;
    }

    if (!primary_output_name || !other_output_name) 
        return;

    printf("DEBUG primary %s, other %s\n", primary_output_name, other_output_name);
    /*
     * TODO: it need to read backup file monitors.xml to get other_output 
     *       (not primary outut) resolution, then set other_output mode at first 
     */
    char *argv[] = {"Deepin XRandR",                                        
                    "--output",                                             
                    other_output_name,                                            
                    "--same-as",                                         
                    primary_output_name};                                             
    xrandr_main(5, argv);                                                   
    xrandr_cleanup();  
}

static void m_set_output_names(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRRConfig *config = NULL;
    GnomeRROutputInfo **output_infos = NULL;
    GnomeRROutput **outputs = NULL;
    char output_name[BUF_SIZE];
    gchar **strv = NULL;
    int count = 0;
    int i = 0;
    size_t output_name_length = 0;

    config = gnome_rr_config_new_current(screen, NULL);
    if (!config) 
        return;

    output_infos = gnome_rr_config_get_outputs(config);
    if (!output_infos) 
        return;

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
    while (outputs[i] && output_infos[i]) {
        memset(output_name, 0, BUF_SIZE);
        if (gnome_rr_output_is_connected(outputs[i])) { 
            sprintf(output_name, 
                    "%s (%s)", 
                    gnome_rr_output_info_get_display_name(output_infos[i]), 
                    gnome_rr_output_get_name(outputs[i]));
        } else 
            strcpy(output_name, "NULL");

        output_name_length = (strlen(output_name) + 1) * sizeof(gchar);
        strv[i] = malloc(output_name_length);
        if (!strv[i]) {
            i++;
            continue;
        }

        memset(strv[i], 0, output_name_length);
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

    if (config) {
        g_object_unref(config);
        config = NULL;
    }
}

int deepin_xrandr_init(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRRConfig *config = NULL;
    char backup_filename[PATH_MAX] = {'\0'}; 
    struct passwd *pw = NULL;

    config = gnome_rr_config_new_current(screen, NULL);
    if (!config) 
        return -1;

    /* TODO: GnomeRRScreen changed event */
    g_signal_connect(screen, "changed", m_screen_changed, settings);
    
    /* TODO: GSettings changed brightness key event */
    g_signal_connect(settings, 
                     "changed::brightness", 
                     m_changed_brightness, 
                     screen);

    m_set_output_names(screen, settings);
    m_set_brightness(screen, settings);

    pw = getpwuid(getuid());
    if (!pw) 
        return -1;

    sprintf(backup_filename, "%s/.config/monitors.xml", pw->pw_dir);
    /* TODO: create monitors.xml if it is not exist */
    if (access(backup_filename, 0) == -1) 
        gnome_rr_config_save(config, NULL);

    m_config_file = g_file_new_for_path(backup_filename);
    if (!m_config_file) 
        return -1;

    m_config_file_monitor = g_file_monitor_file(m_config_file, 
                                                G_FILE_MONITOR_NONE, 
                                                NULL, 
                                                NULL);
    if (!m_config_file_monitor) { 
        return -1;
    }

    /* TODO: GFile changed event */
    g_signal_connect(m_config_file_monitor, 
                     "changed", 
                     m_config_file_changed, 
                     NULL);

    if (config) {
        g_object_unref(config);
        config = NULL;
    }

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
