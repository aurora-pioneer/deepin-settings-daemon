NULL=

plugin_name = idle-delay

if BUILD_TEST_PROGRAMS
noinst_PROGRAMS = \
	test-idle-delay\
	$(NULL)

test_idle_delay_SOURCES= \
	test-idle-delay.c\
	gsd-idle-delay-dbus.h\
	gsd-idle-delay-marshal.h\
	gsd-idle-delay-marshal.c\
	gsd-idle-delay-watcher.h\
	gsd-idle-delay-watcher.c\
	gsd-idle-delay-manager.c\
	gsd-idle-delay-manager.h\
	$(NULL)

test_idle_delay_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"		\
	-DLIBEXECDIR=\""$(libexecdir)"\"			\
	$(AM_CPPFLAGS)

test_idle_delay_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(IDLE_DELAY_CFLAGS)		\
	$(AM_CFLAGS) -g

test_idle_delay_LDADD =		\
	$(top_builddir)/gnome-settings-daemon/libgsd.la	\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(IDLE_DELAY_LIBS)	\
	$(NULL)
endif


plugin_LTLIBRARIES = \
	libidle-delay.la

libidle_delay_la_SOURCES = \
	gsd-idle-delay-dbus.h\
	gsd-idle-delay-marshal.h\
	gsd-idle-delay-marshal.c\
	gsd-idle-delay-watcher.h\
	gsd-idle-delay-watcher.c\
	gsd-idle-delay-manager.h	\
	gsd-idle-delay-manager.c	\
	gsd-idle-delay-plugin.c\
	$(NULL)

libidle_delay_la_CPPFLAGS = \
	-I$(top_srcdir)/plugins/common			\
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\"			\
	$(AM_CPPFLAGS)

libidle_delay_la_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libidle_delay_la_LDFLAGS = 		\
	$(GSD_PLUGIN_LDFLAGS)

libidle_delay_la_LIBADD  = 		\
	$(top_builddir)/plugins/common/libcommon.la	\
	$(SETTINGS_PLUGIN_LIBS)

gsd-idle-delay-marshal.c: gsd-idle-delay-marshal.list gsd-idle-delay-marshal.h
	echo "#include \"gsd-idle-delay-marshal.h\"" > $@ && \
	@GLIB_GENMARSHAL@ $< --prefix=gsd_idle_delay_marshal --body >> $@

gsd-idle-delay-marshal.h: gsd-idle-delay-marshal.list
	@GLIB_GENMARSHAL@ $< --prefix=gsd_idle_delay_marshal --header > $@

plugin_in_files = 		\
	idle-delay.gnome-settings-plugin.in

plugin_DATA = $(plugin_in_files:.gnome-settings-plugin.in=.gnome-settings-plugin)

EXTRA_DIST = 			\
	$(plugin_in_files)

CLEANFILES = 			\
	$(plugin_DATA)

DISTCLEANFILES =		\
	$(plugin_DATA)

@GSD_INTLTOOL_PLUGIN_RULE@
