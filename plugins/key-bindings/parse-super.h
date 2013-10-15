/**
 * Copyright (c) 2011 ~ 2013 Deepin, Inc.
 *               2011 ~ 2013 jouyouyun
 *
 * Author:      jouyouyun <jouyouwen717@gmail.com>
 * Maintainer:  jouyouyun <jouyouwen717@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef __PARSE_SUPER_H__
#define __PARSE_SUPER_H__

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>
#include <X11/XKBlib.h>

typedef struct _XCape_t {
    Display* data_conn;
    Display* ctrl_conn;
    XRecordContext record_ctx;
    KeyCode key;
} XCape_t;

gboolean init_xrecord (void);
void intercept (XPointer user_data, XRecordInterceptData* data);
void insert_table_record (KeySym keysym, gchar* data);
void remove_table_record (KeySym keysym);
void remove_table_all_record (void);
void finalize_xrecord (void);
gboolean is_grabbed (void);

#endif
