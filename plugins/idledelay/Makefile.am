plugin_name = idledelay 

plugin_LTLIBRARIES = \
	libidledelay.la

libidledelay_la_SOURCES = \
	gsd-idledelay-manager.c	\
	gsd-idledelay-manager.h	\
	gsd-idledelay-plugin.c	\
	gsd-idledelay-plugin.h

libidledelay_la_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libidledelay_la_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libidledelay_la_LDFLAGS = 		\
	$(GSD_PLUGIN_LDFLAGS)

libidledelay_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)

plugin_in_files = 		\
	idledelay.gnome-settings-plugin.in

plugin_DATA = $(plugin_in_files:.gnome-settings-plugin.in=.gnome-settings-plugin)

EXTRA_DIST = 			\
	$(plugin_in_files)

CLEANFILES = 			\
	$(plugin_DATA)

DISTCLEANFILES =		\
	$(plugin_DATA)

@GSD_INTLTOOL_PLUGIN_RULE@
