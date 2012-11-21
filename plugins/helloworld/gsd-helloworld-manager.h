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

#ifndef __GSD_HELLOWORLD_MANAGER_H
#define __GSD_HELLOWORLD_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GSD_TYPE_HELLOWORLD_MANAGER         (gsd_helloworld_manager_get_type())
#define GSD_HELLOWORLD_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), GSD_TYPE_HELLOWORLD_MANAGER, GsdHelloWorldManager))
#define GSD_HELLOWORLD_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSD_TYPE_HELLOWORLD_MANAGER, GsdHelloWorldManagerClass))
#define GSD_IS_HELLOWORLD_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), GSD_TYPE_HELLOWORLD_MANAGER))
#define GSD_IS_HELLOWORLD_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE((k), GSD_TYPE_HELLOWORLD_MANAGER))
#define GSD_HELLOWORLD_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GSD_TYPE_HELLOWORLD_MANAGER, GsdHelloWorldManagerClass))
#define GSD_HELLOWORLD_MANAGER_ERROR        (gsd_helloworld_manager_error_quark())

typedef struct GsdHelloWorldManagerPrivate GsdHelloWorldManagerPrivate;

typedef struct
{
    GObject                     parent;
    GsdHelloWorldManagerPrivate *priv;
} GsdHelloWorldManager;

typedef struct
{
    GObjectClass   parent_class;
} GsdHelloWorldManagerClass;

enum
{
    GSD_HELLOWORLD_MANAGER_ERROR_FAILED
};

GType                   gsd_helloworld_manager_get_type            (void);
GQuark                  gsd_helloworld_manager_error_quark         (void);

GsdHelloWorldManager *       gsd_helloworld_manager_new                 (void);
gboolean                gsd_helloworld_manager_start               (GsdHelloWorldManager *manager,
                                                               GError         **error);
void                    gsd_helloworld_manager_stop                (GsdHelloWorldManager *manager);

G_END_DECLS

#endif /* __GSD_HELLOWORLD_MANAGER_H */
