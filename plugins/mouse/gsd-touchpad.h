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

#ifndef _GSD_TOUCHPAD_SYNAPTICS_H
#define _GSD_TOUCHPAD_SYNAPTICS_H

#define SETTINGS_TOUCHPAD_DIR      "org.gnome.settings-daemon.peripherals.touchpad"

/* Touchpad settings */
#define KEY_TOUCHPAD_DISABLE_W_TYPING    "disable-while-typing"
#define KEY_PAD_HORIZ_SCROLL             "horiz-scroll-enabled"
#define KEY_SCROLL_METHOD                "scroll-method"
#define KEY_TAP_TO_CLICK                 "tap-to-click"
#define KEY_TOUCHPAD_ENABLED             "touchpad-enabled"
#define KEY_NATURAL_SCROLL_ENABLED       "natural-scroll"


void        touchpad_apply_settings     (GsdMouseManager *manager, GdkDevice *device);
gboolean    get_touchpad_handedness     (GsdMouseManager *manager);
gboolean    touchpad_has_single_button  (XDevice *device);

void set_tap_to_click (GdkDevice *device, gboolean state, gboolean left_handed);
void set_edge_scroll (GdkDevice *device, GsdTouchpadScrollMethod  method);
void set_horiz_scroll (GdkDevice *device, gboolean   state);
void set_touchpad_disabled (GdkDevice *device);
int set_disable_w_typing (GsdMouseManager *manager, gboolean state);
void ensure_touchpad_active (GsdMouseManager *manager);
void touchpad_callback (GSettings *settings, const gchar *key, GsdMouseManager *manager);
void set_touchpad_enabled (int id);
void set_natural_scroll (GsdMouseManager *manager, GdkDevice *device, gboolean natural_scroll);

#endif /* _GSD_TOUCHPAD_SYNAPTICS_H */
