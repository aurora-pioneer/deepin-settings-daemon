/* 
 * Copyright (C) 2012 ~ 2013 Deepin, Inc.
 *               2012 ~ 2013 Zhai Xiang
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
#include <gtk/gtk.h>  //gtk is initialized in gnome-settings-daemon/main.c

#include "deepin_xrandr.h"

#define BUF_SIZE 1024

static GFile *m_config_file = NULL;
static GFileMonitor *m_config_file_monitor = NULL;
static gboolean m_is_config_file_changed = FALSE;
static gboolean m_is_copy_monitor = TRUE;
static char *m_primary_output_name = NULL;
static char *m_other_output_name = NULL;
static gboolean m_only2_shown = FALSE;
static gboolean m_only1_shown = FALSE;

static void m_config_file_changed(GFileMonitor *monitor, 
                                  GFile *file, 
                                  GFile *other_file, 
                                  GFileMonitorEvent event_type, 
                                  gpointer user_data);
static void m_screen_changed(GnomeRRScreen *screen, gpointer user_data);
static void m_set_output_names(GnomeRRScreen *screen, GSettings *settings);
static void m_set_multi_monitors(GnomeRRScreen *screen, GSettings *settings);
static void m_get_same_mode(GnomeRRMode **primary,                              
                            GnomeRRMode **other,                                
                            char *same);
static void m_use_mirror(char *primary_output_name, 
                         char *other_output_name, 
                         char *same);
static void m_use_extend(GnomeRRScreen* screen, char *primary_output_name, char *other_output_name);
static void m_only_one_shown(char *primary_output_name, 
                             char *other_output_name, 
                             int index);
static void m_settings_changed(GSettings *settings, 
                               gchar *key, 
                               gpointer user_data);
static void m_set_brightness(GnomeRRScreen *screen, GSettings *settings);
static void m_set_rotation(char *rotation);
static void m_parse_configuration(xmlDocPtr doc, 
                                  xmlNodePtr cur, 
                                  GnomeRRScreen *screen);
static void m_parse_output(xmlDocPtr doc, xmlNodePtr cur, gboolean is_clone);

static void m_parse_configuration(xmlDocPtr doc, 
                                  xmlNodePtr cur, 
                                  GnomeRRScreen *screen) 
{
    char *output_name = NULL;                       
    char *clone = NULL;
    GnomeRROutput *output = NULL;
    gboolean is_clone = FALSE;

    cur = cur->xmlChildrenNode;                                                 
    while (cur) {                                                               
        if (!xmlStrcmp(cur->name, (const xmlChar *) "clone")) {                 
            clone = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
            if (strcmp(clone, "yes") == 0) 
                is_clone = TRUE;
            else 
                is_clone = FALSE;
        }                                                                       
        if (!xmlStrcmp(cur->name, (const xmlChar *) "output")) {                
            output_name = xmlGetProp(cur, (const xmlChar *) "name");            
            if (output_name) {                                                  
                output = gnome_rr_screen_get_output_by_name(screen,             
                                                            output_name);          
                if (gnome_rr_output_is_connected(output))                       
                    m_parse_output(doc, cur, is_clone);                                   
            }                                                                   
        }                                                                       
        cur = cur->next;                                                        
    }         
}

static void m_parse_output(xmlDocPtr doc, xmlNodePtr cur, gboolean is_clone) 
{
    char *width = NULL;
    char *height = NULL;
    char buffer[BUF_SIZE] = {'\0'};

    cur = cur->xmlChildrenNode;
    while (cur) {
        if (!xmlStrcmp(cur->name, (const xmlChar *) "width")) {                 
            width = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1); 
        }  
        if (!xmlStrcmp(cur->name, (const xmlChar *) "height")) {                 
            height = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1); 
        }  
        if (!xmlStrcmp(cur->name, (const xmlChar *) "rotation")) {
            m_set_rotation(xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
        }
        cur = cur->next;
    }
    
    if (m_primary_output_name && m_other_output_name && 
        is_clone && m_is_copy_monitor) {
        sprintf(buffer, "%sx%s", width, height);
        m_use_mirror(m_primary_output_name, m_other_output_name, buffer);
    }
}

/* TODO: backup_file xml changed handle */
static void m_config_file_changed(GFileMonitor *monitor, 
                                  GFile *file, 
                                  GFile *other_file, 
                                  GFileMonitorEvent event_type, 
                                  gpointer user_data) 
{
    GnomeRRScreen *screen = (GnomeRRScreen*) user_data;
    GnomeRRConfig *config = NULL;
    char *filename = NULL;
    xmlDocPtr doc = NULL;
    xmlNodePtr cur = NULL;
    char *output_name = NULL;
    GnomeRROutput *output = NULL;

    if (G_FILE_MONITOR_EVENT_CHANGED != event_type) 
        return;

    config = gnome_rr_config_new_current(screen, NULL);

    m_is_config_file_changed = TRUE;

    filename = g_file_get_path(file);
    doc = xmlParseFile(filename);
    if (!doc) 
        return;

    cur = xmlDocGetRootElement(doc);
    if (!cur) {
        xmlFreeDoc(doc);
        return;
    }

    cur = cur->xmlChildrenNode;
    while (cur) { 
        if (!xmlStrcmp(cur->name, (const xmlChar *) "configuration")) {
            m_parse_configuration(doc, cur, screen);
        }
        cur = cur->next;
    }

    if (config) {
        gnome_rr_config_save(config, NULL);
        g_object_unref(config);
        config = NULL;
    }
}

static void m_screen_changed(GnomeRRScreen *screen, gpointer user_data) 
{
    GSettings *settings = (GSettings *) user_data;

    m_set_output_names(screen, settings);
    m_set_multi_monitors(screen, settings);
}

static void m_settings_changed(GSettings *settings, 
                               gchar *key, 
                               gpointer user_data) 
{
    GnomeRRScreen *screen = (GnomeRRScreen *) user_data;
    
    if (strcmp(key, "brightness") == 0) {
        m_set_brightness(screen, settings);
        return;
    }

    if (strcmp(key, "copy-multi-monitors") == 0 || 
        strcmp(key, "extend-multi-monitors") == 0 || 
        strcmp(key, "only-monitor-shown") == 0) {
        m_set_multi_monitors(screen, settings);
        return;
    }
}

static void m_set_brightness(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRRConfig *config = NULL;
    GnomeRROutputInfo **output_infos = NULL;
    GnomeRROutput *output = NULL;
    GError *error = NULL;
    char *output_name = NULL;
    double value = 0.0;
    char buffer[BUF_SIZE];
    int i = 0;
    int backlight_min = 0;
    int backlight_max = 0;
    int backlight = 0;
    
    config = gnome_rr_config_new_current(screen, NULL);
    if (!config) 
        return;

    output_infos = gnome_rr_config_get_outputs(config);
    if (!output_infos) 
        return;

    value = g_settings_get_double(settings, "brightness");
    if (value < 0.1 || value > 1.0) 
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

        memset(buffer, 0, BUF_SIZE);
        sprintf(buffer, "xrandr --output %s --brightness %f", output_name, value);
        system(buffer);

        backlight_min = gnome_rr_output_get_backlight_min(output);
        backlight_max = gnome_rr_output_get_backlight_max(output);
        gnome_rr_output_set_backlight(output, 
                                      (backlight_max - backlight_min) * value, 
                                      &error);
        
        i++;
    }

    if (config) {
        g_object_unref(config);
        config = NULL;
    }
}

static void m_get_same_mode(GnomeRRMode **primary, 
                            GnomeRRMode **other, 
                            char *same) 
{
    int i = 0;
    int j = 0;
    guint primary_width = 0;
    guint primary_height = 0;

    while (primary[i]) {
        primary_width = gnome_rr_mode_get_width(primary[i]);
        primary_height = gnome_rr_mode_get_height(primary[i]);
        
        j = 0;
        while (other[j]) {
            if (gnome_rr_mode_get_width(other[j]) == primary_width && 
                gnome_rr_mode_get_height(other[j]) == primary_height) {
                sprintf(same, "%dx%d", primary_width, primary_height);
                return;
            }
            
            j++;
        }
        
        i++;
    }
}

static void m_set_multi_monitors(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRRConfig *config = NULL;                                               
    GnomeRROutputInfo **output_infos = NULL;                                    
    GnomeRROutput **outputs = NULL;
    GnomeRRMode **primary_mode = NULL;
    GnomeRRMode **other_mode = NULL;
    char *primary_output_name = NULL;
    char *other_output_name = NULL;
    char same_mode[BUF_SIZE] = {'\0'};
    int output_index = 0;
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
            primary_mode = gnome_rr_output_list_modes(outputs[i]);
            primary_output_name = gnome_rr_output_info_get_name(output_infos[i]);
            i++;
            continue;
        }

        other_mode = gnome_rr_output_list_modes(outputs[i]);
        other_output_name = gnome_rr_output_info_get_name(output_infos[i]);
        
        i++;
    }

    if (!primary_output_name || !other_output_name) 
        return;

    m_primary_output_name = primary_output_name;
    m_other_output_name = other_output_name;

    if (g_settings_get_boolean(settings, "copy-multi-monitors")) {
        m_is_copy_monitor = TRUE;
        if (!m_is_config_file_changed) {
            m_get_same_mode(primary_mode, other_mode, same_mode);
            m_use_mirror(primary_output_name, other_output_name, same_mode);
        }
        return;
    } else {
        m_is_copy_monitor = FALSE;
    }

    if (g_settings_get_boolean(settings, "extend-multi-monitors")) {
        m_use_extend(screen, primary_output_name, other_output_name);
        return;
    }

    output_index = g_settings_get_int(settings, "only-monitor-shown");
    if (output_index != 0) {
        m_only_one_shown(primary_output_name, 
                         other_output_name, 
                         output_index);
    }

    if (config) {                                                               
        g_object_unref(config);                                                 
        config = NULL;                                                          
    }   
}

static void m_use_mirror(char *primary_output_name, 
                         char *other_output_name, 
                         char *same) 
{
    /* xrandr --output LVDS --mode 1024x768 
     *        --output VGA-0 --mode 1024x768 --same-as LVDS 
     */
    char buffer[BUF_SIZE] = {'\0'};
    
    sprintf(buffer, 
            "xrandr --output %s --mode %s --output %s --mode %s --same-as %s", 
            primary_output_name, 
            same, 
            other_output_name, 
            same, 
            primary_output_name);
    system(buffer);
}

static void m_use_extend(GnomeRRScreen* screen, char *primary_output_name, char *other_output_name) 
{
    /* xrandr --output LVDS --auto --output VGA-0 --auto --right-of LVDS */
    char buffer[BUF_SIZE] = {'\0'};

    //FIXME: temporary hack to align two outputs
    GdkScreen* gdk_screen = gdk_screen_get_default ();
    gint screen_width = gdk_screen_get_width (gdk_screen);
    gint screen_height = gdk_screen_get_height (gdk_screen);
    GnomeRROutput* primary_output = gnome_rr_screen_get_output_by_name (screen, primary_output_name);
    GnomeRROutput* other_output = gnome_rr_screen_get_output_by_name (screen, other_output_name);
    GnomeRRMode* primary_mode = gnome_rr_output_get_current_mode (primary_output);
    GnomeRRMode* other_mode = gnome_rr_output_get_current_mode (other_output);
    gint primary_width = gnome_rr_mode_get_width (primary_mode);
    gint primary_height = gnome_rr_mode_get_height (primary_mode);
    gint other_width = gnome_rr_mode_get_width (other_mode);
    gint other_height = gnome_rr_mode_get_height (other_mode);
    
    gint primary_x = 0;
    gint primary_y = screen_height - primary_height;
    guint other_x = primary_width;
    guint other_y = screen_height -other_height;

    sprintf(buffer, 
            "xrandr --output %s --auto --pos %dx%d --output %s --auto --pos %dx%d", 
            primary_output_name,  primary_x, primary_y,
            other_output_name, other_x, other_y); 
    system(buffer);
}

static void m_only_one_shown(char *primary_output_name, 
                             char *other_output_name, 
                             int index) 
{
    /* xrandr --output LVDS --auto --output VGA-0 --off  */
    if (index == 1 && !m_only1_shown) {
        char buffer[BUF_SIZE] = {'\0'};
        sprintf(buffer, "xrandr --output %s --primary", primary_output_name);
        printf("DEBUG %s\n", buffer);
        system(buffer);
        memset(buffer, 0, BUF_SIZE);
        sprintf(buffer, 
                "xrandr --output %s --auto --output %s --off", 
                primary_output_name, 
                other_output_name);
        printf("DEBUG %s\n", buffer);
        system(buffer);
        m_only1_shown = TRUE;
        m_only2_shown = FALSE;
        return;
    }

    if (index == 2 && !m_only2_shown) {
        char buffer[BUF_SIZE] = {'\0'};

        sprintf(buffer, "xrandr --output %s --primary", primary_output_name);
        printf("DEBUG %s\n", buffer);
        system(buffer);
        memset(buffer, 0, BUF_SIZE);
        sprintf(buffer, 
                "xrandr --output %s --auto --output %s --off", 
                other_output_name, 
                primary_output_name);
        printf("DEBUG %s\n", buffer);
        system(buffer);
        m_only1_shown = FALSE;
        m_only2_shown = TRUE;
        return;
    }
}

static void m_set_output_names(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRRConfig *config = NULL;
    GnomeRROutputInfo **output_infos = NULL;
    GnomeRROutput **outputs = NULL;
    char output_name[BUF_SIZE];
    char *rr_output_name = NULL;
    gchar **strv = NULL;
    int count = 0;
    int i = 0;
    size_t output_name_length = 0;
    gboolean is_laptop = FALSE;

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
            rr_output_name = gnome_rr_output_get_name(outputs[i]);
            if (strstr(rr_output_name, "LVDS"))
                is_laptop = TRUE;
            if (strcmp(rr_output_name, "default") == 0) {
                sprintf(output_name, "%s (%s)", "PC", rr_output_name);
            } else {
                sprintf(output_name, 
                        "%s (%s)", 
                        gnome_rr_output_info_get_display_name(output_infos[i]), 
                        gnome_rr_output_get_name(outputs[i]));
            }
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
  
    g_settings_set_boolean(settings, "is-laptop", is_laptop);
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

static void m_set_rotation(char *rotation) 
{
    char buffer[BUF_SIZE] = {'\0'};
    
    sprintf(buffer, "xrandr -o %s", rotation);
    system(buffer);
}

int deepin_xrandr_init(GnomeRRScreen *screen, GSettings *settings) 
{
    GnomeRRConfig *config = NULL;
    char backup_filename[PATH_MAX] = {'\0'}; 
    struct passwd *pw = NULL;

    config = gnome_rr_config_new_current(screen, NULL);
    if (!config) 
        return -1;

    m_is_copy_monitor = g_settings_get_boolean(settings, "copy-multi-monitors");

    /* TODO: GnomeRRScreen changed event */
    g_signal_connect(screen, "changed", m_screen_changed, settings);
    
    /* TODO: GSettings changed event */
    g_signal_connect(settings, 
                     "changed", 
                     m_settings_changed, 
                     screen);

    m_set_output_names(screen, settings);
    m_set_multi_monitors(screen, settings);

    pw = getpwuid(getuid());
    if (!pw) 
        return -1;

    sprintf(backup_filename, "%s/.config/monitors.xml", pw->pw_dir);
    if (access(backup_filename, 0) == 0) 
        remove(backup_filename);

    gnome_rr_config_save(config, NULL);

    m_set_brightness(screen, settings);

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
                     screen);

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
