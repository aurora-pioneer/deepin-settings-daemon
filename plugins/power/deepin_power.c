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
#include <limits.h>
#include <pwd.h>
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

static const char m_powers_plan_xml[] = 
"<powers version=\"1\">\n"
"  <configuration>\n"
"    <plan name=\"default\">\n"
"      <close-monitor>600</close-monitor>\n"
"      <suspend>600</suspend>\n"
"    </plan>\n"
"    <plan name=\"saving\">\n"                                                   
"      <close-monitor>60</close-monitor>\n"                                      
"      <suspend>60</suspend>\n"                                                  
"    </plan>\n"
"    <plan name=\"high-performance\">\n"                                                   
"      <close-monitor>0</close-monitor>\n"                                      
"      <suspend>0</suspend>\n"                                                  
"    </plan>\n"
"  </configuration>\n"
"</powers>\n";

static GFile *m_config_file = NULL;
static GFileMonitor *m_config_file_monitor = NULL;

static void m_config_file_changed(GFileMonitor *monitor, 
                                  GFile *file, 
                                  GFile *other_file, 
                                  GFileMonitorEvent event_type, 
                                  gpointer user_data);
static void m_settings_changed(GSettings *settings, 
                               gchar *key, 
                               gpointer user_data);

static void m_config_file_changed(GFileMonitor *monitor, 
                                  GFile *file, 
                                  GFile *other_file, 
                                  GFileMonitorEvent event_type, 
                                  gpointer user_data) 
{
    char *filename = NULL;
    xmlDocPtr doc = NULL;
    xmlNodePtr cur = NULL;

    if (G_FILE_MONITOR_EVENT_CHANGED != event_type) 
        return;

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
    /*
    while (cur) { 
        if (!xmlStrcmp(cur->name, (const xmlChar *) "configuration")) {
        }
        cur = cur->next;
    }
    */
}

static void m_settings_changed(GSettings *settings, 
                               gchar *key, 
                               gpointer user_data) 
{
}

int deepin_power_init(GSettings *settings) 
{
    char backup_filename[PATH_MAX] = {'\0'}; 
    struct passwd *pw = NULL;
    FILE *fptr = NULL;
    
    /* TODO: GSettings changed event */
    g_signal_connect(settings, 
                     "changed", 
                     m_settings_changed, 
                     NULL);

    pw = getpwuid(getuid());
    if (!pw) 
        return -1;

    sprintf(backup_filename, "%s/.config/powers.xml", pw->pw_dir);
    if (access(backup_filename, 0) != 0) {
        fptr = fopen(backup_filename, "w+");
        if (fptr == NULL) 
            return -1;
        fwrite(m_powers_plan_xml, 1, strlen(m_powers_plan_xml), fptr);
        fflush(fptr);
        fclose(fptr);
        fptr = NULL;
    }

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

    return 0;
}

void deepin_power_cleanup() 
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
