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
//similar to wacom case, for those who don't have thinkpad, we still have this setting

#ifndef _GSD_MM_TRACKPOINT_H_
#define _GSD_MM_TRACKPOINT_H_

#define SETTINGS_TRACKPOINT_DIR      "org.gnome.settings-daemon.peripherals.trackpoint"

/* Touchpad settings */
//TODO: which evdev parameters we want to set.


void trackpoint_apply_settings (GsdMouseManager *manager, GdkDevice *device);

void trackpoint_callback (GSettings *settings, const gchar *key, GsdMouseManager *manager);

#endif /* _GSD_MM_TRACKPOINT_H_ */
