/*
 * Copyright (C) 2012 Deepin, Inc.
 *               2012 Zhai Xiang <zhaixiang@linuxdeepin.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <canberra-gtk.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-rr-config.h>

#include "gnome-settings-profile.h"
#include "gnome-settings-session.h"
#include "gsd-enums.h"
#include "gsd-helloworld-manager.h"

#define GSD_HELLOWORLD_SETTINGS_SCHEMA               "com.linuxdeepin.settings-daemon.plugins.helloworld"

enum {
        PROP_0,
};

static void     gsd_helloworld_manager_class_init  (GsdHelloWorldManagerClass *klass);
static void     gsd_helloworld_manager_init        (GsdHelloWorldManager      *helloworld_manager);
static void     gsd_helloworld_manager_finalize    (GObject              *object);

G_DEFINE_TYPE (GsdHelloWorldManager, gsd_helloworld_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

GQuark
gsd_helloworld_manager_error_quark (void)
{
        static GQuark quark = 0;
        if (!quark)
                quark = g_quark_from_static_string ("gsd_helloworld_manager_error");
        return quark;
}

static void gsd_helloworld_manager_class_init(GsdHelloWorldManagerClass *klass)
{
    GObjectClass   *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = gsd_helloworld_manager_finalize;
}

gboolean gsd_helloworld_manager_start(GsdHelloWorldManager *manager, 
                                      GError **error)
{
    g_debug("Starting helloworld manager");
        
    return TRUE;
}

void gsd_helloworld_manager_stop(GsdHelloWorldManager *manager)
{
    g_debug("Stopping helloworld manager");
}

static void gsd_helloworld_manager_init(GsdHelloWorldManager *manager)
{
    manager->priv = GSD_HELLOWORLD_MANAGER_GET_PRIVATE (manager);
}

static void gsd_helloworld_manager_finalize(GObject *object)
{
    GsdHelloWorldManager *manager;

    manager = GSD_HELLOWORLD_MANAGER(object);

    g_return_if_fail(manager->priv != NULL);

    G_OBJECT_CLASS(gsd_helloworld_manager_parent_class)->finalize(object);
}

GsdHelloWorldManager *gsd_helloworld_manager_new()
{
    if (manager_object != NULL) {
        g_object_ref(manager_object);
    } else {
        manager_object = g_object_new(GSD_TYPE_HELLOWORLD_MANAGER, NULL);
        g_object_add_weak_pointer(manager_object, 
                                  (gpointer *) &manager_object);
    }
    return GSD_HELLOWORLD_MANAGER (manager_object);
}
