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
#ifndef _GSD_MM_DEVICE_H_
#define _GSD_MM_DEVICE_H_

#include "gsd-mouse-manager.h"

/* Keys for both touchpad and mouse */
#define KEY_LEFT_HANDED         "left-handed"                /* a boolean for mouse, an enum for touchpad */
#define KEY_MOTION_ACCELERATION "motion-acceleration"
#define KEY_MOTION_THRESHOLD    "motion-threshold"

typedef enum {
    GSD_MM_DEVICE_TYPE_MOUSE,
    GSD_MM_DEVICE_TYPE_TOUCHPAD,
    GSD_MM_DEVICE_TYPE_TRACKPOINT
} GsdMMDeviceType;

typedef struct GsdMouseManagerPrivate
{
        GHashTable *blacklist;
        guint start_idle_id;

        GdkDeviceManager *device_manager;
        guint device_added_id;
        guint device_removed_id;

        //1. mouse
        GSettings *mouse_settings;
        GSettings *mouse_a11y_settings;
        gboolean mousetweaks_daemon_running;
        gboolean locate_pointer_spawned;
        GPid locate_pointer_pid;

        //2. touchpad
        GSettings *touchpad_settings;
        gboolean syndaemon_spawned;
        GPid syndaemon_pid;

        //3. trackpoint
        GSettings *trackpoint_settings;
} GsdMouseManagerPrivate;

void                setup_device_manager        (GsdMouseManager *manager);
gboolean            device_is_ignored           (GsdMouseManager *manager,  GdkDevice *device);
GsdMMDeviceType     device_get_type             (GdkDevice* device);
void                device_apply_settings       (GsdMouseManager *manager, GdkDevice *device);

XDevice*            open_gdk_device             (GdkDevice *device);


void                device_set_motion           (GsdMouseManager *manager, GdkDevice *device, GSettings* settings);
void                device_set_left_handed      (GsdMouseManager *manager, GdkDevice *device, gboolean left_handed);

#endif /* _GSD_MM_DEVICE_H_*/
