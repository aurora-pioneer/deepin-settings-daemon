plugin_name = key-bindings

plugin_LTLIBRARIES = \
	libkey-bindings.la

libkey_bindings_la_SOURCES = 		\
	gsd-key-bindings-manager.c	\
	gsd-key-bindings-manager.h	\
	gsd-key-bindings-plugin.c	\
	gsd-key-bindings-plugin.h

libkey_bindings_la_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libkey_bindings_la_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libkey_bindings_la_LDFLAGS = 		\
	$(GSD_PLUGIN_LDFLAGS)

libkey_bindings_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)

plugin_in_files = 		\
	key-bindings.gnome-settings-plugin.in

plugin_DATA = $(plugin_in_files:.gnome-settings-plugin.in=.gnome-settings-plugin)

EXTRA_DIST = 			\
	$(plugin_in_files)

CLEANFILES = 			\
	$(plugin_DATA)

DISTCLEANFILES =		\
	$(plugin_DATA)

@GSD_INTLTOOL_PLUGIN_RULE@

# override to _not_ install the test plugin
# do not copy into your plugin
install-pluginDATA:
install-pluginLTLIBRARIES:
