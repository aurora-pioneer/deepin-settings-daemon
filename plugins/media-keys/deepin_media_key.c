/* 
 * Copyright (C) 2013 Deepin, Inc.
 *               2013 Zhai Xiang
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

#include <stdlib.h>
#include <gio/gio.h>
#include "deepin_media_key.h"

typedef struct {
    double brightness;
} deepin_brightness_t;

static GSettings *m_xrandr_settings = NULL;
static pthread_t m_thread;
static pthread_mutex_t m_mutex;

static void *m_set_brightness(void *argv);

int deepin_media_key_init(void) 
{
    m_xrandr_settings = g_settings_new("org.gnome.settings-daemon.plugins.xrandr");

    pthread_mutex_init(&m_mutex, NULL);

    return 0;
}

void deepin_media_key_cleanup(void) 
{
    if (m_xrandr_settings) {
        g_object_unref(m_xrandr_settings);
        m_xrandr_settings = NULL;
    }

    pthread_mutex_destroy(&m_mutex);
}

static void *m_set_brightness(void *argv) 
{
    pthread_mutex_lock(&m_mutex);
    deepin_brightness_t *deepin_brightness_obj = (deepin_brightness_t *)argv;
    double brightness = deepin_brightness_obj->brightness;
    g_settings_set_double(m_xrandr_settings, "brightness", brightness);
    g_settings_sync();
    free(deepin_brightness_obj);
    deepin_brightness_obj = NULL;
    pthread_mutex_unlock(&m_mutex);
    /*return NULL;*/
}

void deepin_media_key_set_brightness(double brightness) 
{
    deepin_brightness_t *deepin_brightness_obj = NULL;
    deepin_brightness_obj = malloc(sizeof(deepin_brightness_t));
    deepin_brightness_obj->brightness = brightness;
    m_thread = pthread_create(&m_thread, NULL, m_set_brightness, (void *)deepin_brightness_obj);
}
