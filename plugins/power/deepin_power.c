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
#include <pwd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

static const char m_powers_plan_xml[] = 
"<powers version=\"1\">\n"
"  <configuration>\n"
"    <plan name=\"balance\">\n"
"      <close-monitor>600</close-monitor>\n"
"      <suspend>0</suspend>\n"
"    </plan>\n"
"    <plan name=\"saving\">\n"                                                   
"      <close-monitor>300</close-monitor>\n"                                      
"      <suspend>900</suspend>\n"                                                  
"    </plan>\n"
"    <plan name=\"high-performance\">\n"                                                   
"      <close-monitor>0</close-monitor>\n"                                      
"      <suspend>0</suspend>\n"                                                  
"    </plan>\n"
"    <plan name=\"customized\">\n"
"      <close-monitor>600</close-monitor>\n"
"      <suspend>600</suspend>\n"
"    </plan>\n"
"  </configuration>\n"
"</powers>\n";

static GSettings *m_session_settings = NULL;
static char *m_backup_filename = NULL;

static void m_parse_plan(xmlDocPtr doc, xmlNodePtr cur, GSettings *settings);
static void m_parse_configuration(xmlDocPtr doc, 
                                  xmlNodePtr cur, 
                                  GSettings *settings, 
                                  char *current_plan);
static void m_settings_changed(GSettings *settings, 
                               gchar *key, 
                               gpointer user_data);

static void m_parse_plan(xmlDocPtr doc, xmlNodePtr cur, GSettings *settings)    
{                                                                               
    char *close_monitor = NULL;                                                         
    char *suspend = NULL;
    int close_monitor_value = 0;
    int suspend_value = 0;    
                                                                                
    cur = cur->xmlChildrenNode;                                                 
    while (cur) {                                                               
        if (!xmlStrcmp(cur->name, (const xmlChar *) "close-monitor")) {                 
            close_monitor = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);         
        }                                                                       
        if (!xmlStrcmp(cur->name, (const xmlChar *) "suspend")) {                
            suspend = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);        
        }                                                                       
        cur = cur->next;                                                        
    }
    close_monitor_value = atoi(close_monitor);
    suspend_value = atoi(suspend);
    g_settings_set_int(settings, "sleep-display-ac", close_monitor_value);
    g_settings_set_int(settings, "sleep-display-battery", close_monitor_value);
    g_settings_set_int(settings, "sleep-inactive-ac-timeout", suspend_value);
    g_settings_set_int(settings, "sleep-inactive-battery-timeout", suspend_value);
    g_settings_set_uint(m_session_settings, "idle-delay", close_monitor_value);
    if (close_monitor_value == 0) {
        g_settings_set_boolean(settings, "idle-dim-battery", FALSE);
    } else {
        g_settings_set_boolean(settings, "idle-dim-battery", TRUE);
    }
    
    g_settings_sync();

    xmlFree (close_monitor);
    xmlFree (suspend);
}

static void m_parse_configuration(xmlDocPtr doc,                                
                                  xmlNodePtr cur,                               
                                  GSettings *settings, 
                                  char *current_plan)                        
{                                                                               
    char *plan_name = NULL;                                                         
                                                                                 
    cur = cur->xmlChildrenNode;                                                  
    while (cur) {                                                                
        if (!xmlStrcmp(cur->name, (const xmlChar *) "plan")) {                
            plan_name = xmlGetProp(cur, (const xmlChar *) "name");            
            if (strcmp(plan_name, current_plan) == 0) {
                m_parse_plan(doc, cur, settings);
                break;
            }                
        }
        cur = cur->next;                                                        
    }                                                                           

    xmlFree (plan_name);
}

static void m_settings_changed(GSettings *settings, 
                               gchar *key, 
                               gpointer user_data) 
{
    if (strcmp(key, "current-plan") != 0) 
        return;

    deepin_power_using_current_plan(settings);
}

int deepin_power_init(GSettings *settings) 
{
    struct passwd *pw = NULL;
    FILE *fptr = NULL;

    m_session_settings = g_settings_new("org.gnome.desktop.session");

    pw = getpwuid(getuid());
    if (!pw) 
        return -1;
    m_backup_filename = malloc(PATH_MAX * sizeof(char));
    if (m_backup_filename == NULL) 
        return -1;
    memset(m_backup_filename, 0, PATH_MAX);
    snprintf(m_backup_filename, PATH_MAX, "%s/.config/powers.xml", pw->pw_dir);
    if (access(m_backup_filename, 0) != 0) {
        fptr = fopen(m_backup_filename, "w+");
        if (fptr == NULL) 
            return -1;
        fwrite(m_powers_plan_xml, 1, strlen(m_powers_plan_xml), fptr);
        fflush(fptr);
        fclose(fptr);
        fptr = NULL;
    }

    g_signal_connect(settings,                                                  
                     "changed",                                                 
                     m_settings_changed,                                        
                     NULL);

    return 0;
}

void deepin_power_using_saving_plan(GSettings *settings) 
{
    int close_monitor_value = 300;
    int suspend_value = 900;

    g_settings_set_int(settings, "sleep-display-ac", close_monitor_value);         
    g_settings_set_int(settings, "sleep-display-battery", close_monitor_value); 
    g_settings_set_int(settings, "sleep-inactive-ac-timeout", suspend_value);   
    g_settings_set_int(settings, "sleep-inactive-battery-timeout", suspend_value);
    g_settings_set_uint(m_session_settings, "idle-delay", close_monitor_value);
    g_settings_set_boolean(settings, "idle-dim-battery", TRUE);
    g_settings_sync();
}

void deepin_power_using_current_plan(GSettings *settings) 
{
    //gchar *current_plan = NULL;                                                 
    xmlDocPtr doc = NULL;                                                       
    xmlNodePtr cur = NULL;                                                      
                                                                                
    //fixed by Long Wei 
    //current_plan = g_settings_get_string(settings, "current-plan");             
                                                                                
    doc = xmlParseFile(m_backup_filename);                                      
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
            gchar *current_plan = g_settings_get_string(settings, "current-plan");             
            m_parse_configuration(doc, cur, settings, current_plan);                
            g_free (current_plan);
        }                                                                       
        cur = cur->next;                                                        
    }

    //fixed, free xml doc and node
    cur = xmlDocGetRootElement(doc);                                            
    if (cur != NULL) 
        xmlFreeNode (cur);
    //if (doc != NULL)
     //   xmlFreeDoc (doc);
}

void deepin_power_cleanup() 
{
    if (m_session_settings) {
        g_object_unref(m_session_settings);
        m_session_settings = NULL;
    }
    if (m_backup_filename) {
        xmlDocPtr doc = xmlParseFile(m_backup_filename);                                      
        if (doc != NULL) {
            xmlNodePtr cur = xmlDocGetRootElement(doc);                                            
            if (cur != NULL) {
                xmlFreeNode (cur);
            }
            xmlFreeDoc (doc);
        }
        free(m_backup_filename);
        m_backup_filename = NULL;
    }                                                                           
    xmlCleanupParser();
}
