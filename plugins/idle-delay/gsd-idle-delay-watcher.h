/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Linux Deepin Inc.
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
/*
 *	watch for gnome-session presence status change event.
 *
 */
#ifndef _GSD_IDLE_DELAY_WATCHER_H_
#define _GSD_IDLE_DELAY_WATCHER_H_

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GSD_TYPE_IDLE_DELAY_WATCHER         (gsd_idle_delay_watcher_get_type ())
#define GSD_IDLE_DELAY_WATCHER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSD_TYPE_IDLE_DELAY_WATCHER, GsdIdleDelayWatcher))
#define GSD_IDLE_DELAY_WATCHER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSD_TYPE_IDLE_DELAY_WATCHER, GsdIdleDelayWatcherClass))
#define GSD_IS_IDLE_DELAY_WATCHER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSD_TYPE_IDLE_DELAY_WATCHER))
#define GSD_IS_IDLE_DELAY_WATCHER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSD_TYPE_IDLE_DELAY_WATCHER))
#define GSD_IDLE_DELAY_WATCHER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSD_TYPE_IDLE_DELAY_WATCHER, GsdIdleDelayWatcherClass))

typedef struct GsdIdleDelayWatcherPrivate GsdIdleDelayWatcherPrivate;

typedef struct
{
        GObject           		parent;
        GsdIdleDelayWatcherPrivate	*priv;
} GsdIdleDelayWatcher;

typedef struct
{
        GObjectClass      parent_class;

        gboolean          (* idle_changed)        (GsdIdleDelayWatcher *watcher,
                                                   gboolean   is_idle);
        gboolean          (* idle_notice_changed) (GsdIdleDelayWatcher *watcher,
                                                   gboolean   in_effect);
} GsdIdleDelayWatcherClass;

GType       		gsd_idle_delay_watcher_get_type         (void);

GsdIdleDelayWatcher* 	gsd_idle_delay_watcher_new              (void);
gboolean    		gsd_idle_delay_watcher_set_enabled      (GsdIdleDelayWatcher *watcher, gboolean enabled);
gboolean    		gsd_idle_delay_watcher_get_enabled      (GsdIdleDelayWatcher *watcher);
gboolean    		gsd_idle_delay_watcher_set_active       (GsdIdleDelayWatcher *watcher, gboolean active);
gboolean    		gsd_idle_delay_watcher_get_active       (GsdIdleDelayWatcher *watcher);

G_END_DECLS

#endif /*_GSD_IDLE_DELAY_WATCHER_H_*/
