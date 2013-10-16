/* 
 * Copyright (C) 2012 ~ 2013 Deepin, Inc.
 *               2012 ~ 2013 Zhai Xiang
 *
 * Author:     Zhai Xiang <zhaixiang@linuxdeepin.com>
 * Maintainer: Zhai Xiang <zhaixiang@linuxdeepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DEEPIN_XRANDR_H
#define DEEPIN_XRANDR_H

#define GNOME_DESKTOP_USE_UNSTABLE_API                                          
                                                                                
#include <libgnome-desktop/gnome-rr-config.h>                                   
#include <libgnome-desktop/gnome-rr.h>

int deepin_xrandr_init(GnomeRRScreen *screen, GSettings *settings);
void deepin_xrandr_cleanup(void);


#endif /* DEEPIN_XRANDR_H */
