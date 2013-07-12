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

#ifdef  _GSD_TOUCHPAD_SYNAPTICS_H
#define _GSD_TOUCHPAD_SYNAPTICS_H

#define SETTINGS_TOUCHPAD_DIR      "org.gnome.settings-daemon.peripherals.touchpad"

/* Touchpad settings */
#define KEY_TOUCHPAD_DISABLE_W_TYPING    "disable-while-typing"
#define KEY_PAD_HORIZ_SCROLL             "horiz-scroll-enabled"
#define KEY_SCROLL_METHOD                "scroll-method"
#define KEY_TAP_TO_CLICK                 "tap-to-click"
#define KEY_TOUCHPAD_ENABLED             "touchpad-enabled"
#define KEY_NATURAL_SCROLL_ENABLED       "natural-scroll"

gboolean touchpad_has_single_button (XDevice *device);

#endif /* _GSD_TOUCHPAD_SYNAPTICS_H */
