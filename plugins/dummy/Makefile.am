plugin_name = dummy

plugin_LTLIBRARIES = \
	libdummy.la

libdummy_la_SOURCES = 		\
	gsd-dummy-manager.c	\
	gsd-dummy-manager.h	\
	gsd-dummy-plugin.c	\
	gsd-dummy-plugin.h

libdummy_la_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libdummy_la_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libdummy_la_LDFLAGS = 		\
	$(GSD_PLUGIN_LDFLAGS)

libdummy_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)

if BUILD_TEST_PROGRAMS
noinst_PROGRAMS =\
	test-dummy

#NOTE: test programs should not list **-plugin.c/h file in
#      the source list.
test_dummy_SOURCES =\
	test-dummy.c\
	gsd-dummy-manager.c\
	gsd-dummy-manager.h
test_dummy_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"		\
	-DLIBEXECDIR=\""$(libexecdir)"\"			\
	$(AM_CPPFLAGS)
test_dummy_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(DUMMY_CFLAGS)		\
	$(AM_CFLAGS) -g
test_dummy_LDADD =		\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(top_builddir)/gnome-settings-daemon/libgsd.la	\
	$(DUMMY_LIBS)
endif

plugin_in_files = 		\
	dummy.gnome-settings-plugin.in

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
