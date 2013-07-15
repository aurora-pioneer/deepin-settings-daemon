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

#ifndef _GSD_MM_MOUSE_H_
#define _GSD_MM_MOUSE_H_

#define SETTINGS_MOUSE_DIR         "org.gnome.settings-daemon.peripherals.mouse"

/* Mouse settings */
#define KEY_LOCATE_POINTER               "locate-pointer"
#define KEY_DWELL_CLICK_ENABLED          "dwell-click-enabled"
#define KEY_SECONDARY_CLICK_ENABLED      "secondary-click-enabled"
#define KEY_MIDDLE_BUTTON_EMULATION      "middle-button-enabled"

void mouse_apply_settings   (GsdMouseManager *manager, GdkDevice *device);

void set_middle_button      (GsdMouseManager *manager, GdkDevice *device, gboolean middle_button);
void set_mousetweaks_daemon (GsdMouseManager *manager, gboolean dwell_click_enabled, gboolean secondary_click_enabled);
void mouse_callback         (GSettings *settings, const gchar *key, GsdMouseManager *manager);
void set_locate_pointer     (GsdMouseManager *manager, gboolean state);

#endif /*_GSD_MM_MOUSE_H_*/
