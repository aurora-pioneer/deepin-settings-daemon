/* vim: set noet ts=8 sts=8 sw=8 :
 *
 * Copyright © 2013 Linux Deepin Inc. 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#ifndef _GSD_IDLE_DELAY_MISC_H_
#define _GSD_IDLE_DELAY_MISC_H_

#define MSEC_PER_SEC			1000
/* GSettings */
#define IDLE_DELAY_SCHEMA		"org.gnome.settings-daemon.plugins.idle-delay"
#define IDLE_DELAY_KEY_BRIGHTNESS	"brightness"
#define IDLE_DELAY_KEY_TIMEOUT		"timeout"

#define XRANDR_KEY_BRIGHTNESS		"brightness"

/* DBus */
#define DBUS_SERVICE                    "org.freedesktop.DBus"
#define DBUS_PATH                       "/org/freedesktop/DBus"
#define DBUS_INTERFACE                  "org.freedesktop.DBus"
#define DBUS_PROP_INTERFACE		"org.freedesktop.DBus.Properties"

/* Gnome Screensaver */
#define GS_SERVICE                      "org.gnome.ScreenSaver"
#define GS_PATH                         "/org/gnome/ScreenSaver"
#define GS_INTERFACE                    "org.gnome.ScreenSaver"

/* Gnome Session Manager */
#define GSM_SERVICE                     "org.gnome.SessionManager"
#define GSM_PATH                        "/org/gnome/SessionManager"
#define GSM_INTERFACE                   "org.gnome.SessionManager"

#define GSM_PRESENCE_PATH               GSM_PATH "/Presence"
#define GSM_PRESENCE_INTERFACE          GSM_INTERFACE ".Presence"

#endif/*_GSD_IDLE_DELAY_MISC_H_*/
