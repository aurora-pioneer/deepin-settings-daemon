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

#ifndef __GSD_POWER_MOVIE_H__
#define __GSD_POWER_MOVIE_H__

#include <gio/gio.h>

#define BALANCE             "balance"
#define SAVING              "saving"
#define HIGH_PERFORMANCER   "high-performance"

void init_power_movie (void);
void finalize_reset_power (void);
gchar* get_user_power_plan (void);
int set_power_plan (const gchar* plan);
gboolean power_setting_is_change (gulong pid);

#endif
