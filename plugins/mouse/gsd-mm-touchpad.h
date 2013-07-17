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

#ifndef _GSD_MM_TOUCHPAD_H_
#define _GSD_MM_TOUCHPAD_H_

#define KEY_TOUCHPAD_DISABLE_W_TYPING    "disable-while-typing" //FIXME: needed by gsd-mm-device.c
#define KEY_TOUCHPAD_ENABLED             "touchpad-enabled"     //FIXME: needed by gsd-mouse-manager.c

void        touchpad_init_settings      (GsdMouseManager *manager);
void        touchpad_apply_settings     (GsdMouseManager *manager, GdkDevice *device);
void        touchpad_callback           (GSettings *settings, const gchar *key, GsdMouseManager *manager);
gboolean    get_touchpad_handedness     (GsdMouseManager *manager);
gboolean    touchpad_has_single_button  (XDevice *device);
void        touchpad_ensure_active      (GsdMouseManager *manager);

void        touchpad_set_enabled        (int id);

int         set_disable_w_typing        (GsdMouseManager *manager, gboolean state);

#endif /* _GSD_MM_TOUCHPAD_H_ */
