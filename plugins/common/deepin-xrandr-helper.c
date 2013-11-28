#include "deepin-xrandr-helper.h"
#include <glib.h>
#include <gio/gio.h>


void deepin_xrandr_set_brightness(double value, gboolean effect_gsetting)
{
    if (effect_gsetting) {
	GSettings* s = g_settings_new("org.gnome.settings-daemon.plugins.xrandr");
	g_settings_set_double(s, "brightness", value);
	g_object_unref(s);
    } else {
	GDBusProxy* proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
		0,
		NULL,
		"org.gnome.SettingsDaemon",
		"/org/gnome/SettingsDaemon/XRANDR",
		"org.gnome.SettingsDaemon.XRANDR_2",
		NULL,
		NULL);

	g_dbus_proxy_call(proxy, "SetBrightness",
		g_variant_new("(d)", value), //this is an float reference ,so we don't need unref it
		G_DBUS_CALL_FLAGS_NONE,
		-1, NULL, NULL, NULL);
	g_object_unref(proxy);
    }
}
double deepin_xrandr_get_brightness()
{
    GSettings* settings = g_settings_new("org.gnome.settings-daemon.plugins.xrandr");
    double value = g_settings_get_double(settings, "brightness");
    g_object_unref(settings);
    return value;
}
