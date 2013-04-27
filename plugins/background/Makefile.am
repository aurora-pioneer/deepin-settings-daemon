NULL =

plugin_name = background

if BUILD_TEST_PROGRAMS
noinst_PROGRAMS = 			\
	test-background			\
	$(NULL)

test_background_SOURCES = 		\
	test-background.c		\
	gsd-background-manager.h	\
	gsd-background-manager-private.h \
	gsd-background-manager.c	\
	background_util.c		\
	background_util.h		\
	$(NULL)

test_background_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"		\
	-DLIBEXECDIR=\""$(libexecdir)"\"			\
	$(AM_CPPFLAGS)

test_background_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(BACKGROUND_CFLAGS)		\
	$(AM_CFLAGS) -g

test_background_LDADD =		\
	$(top_builddir)/gnome-settings-daemon/libgsd.la	\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(BACKGROUND_LIBS)	\
	$(NULL)
endif

plugin_LTLIBRARIES = \
	libbackground.la		\
	$(NULL)

libbackground_la_SOURCES =		\
	gsd-background-plugin.h		\
	gsd-background-plugin.c		\
	gsd-background-manager.h	\
	gsd-background-manager-private.h \
	gsd-background-manager.c	\
	background_util.c		\
	background_util.h		\
	$(NULL)

libbackground_la_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-I$(top_srcdir)/plugins/background/libbackground   \
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"		\
	-DLIBEXECDIR=\""$(libexecdir)"\"			\
	$(AM_CPPFLAGS)

libbackground_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(BACKGROUND_CFLAGS)		\
	$(AM_CFLAGS)

libbackground_la_LDFLAGS =		\
	$(GSD_PLUGIN_LDFLAGS)		\
	$(NULL)

libbackground_la_LIBADD  =		\
	$(SETTINGS_PLUGIN_LIBS)		\
	$(BACKGROUND_LIBS)		\
	$(NULL)

plugin_in_files =		\
	background.gnome-settings-plugin.in	\
	$(NULL)

plugin_DATA = $(plugin_in_files:.gnome-settings-plugin.in=.gnome-settings-plugin)

libexec_PROGRAMS = gsd-background-helper

gsd_background_helper_SOURCES = 	\
		background_helper.c 	\
		gaussianiir2d.c

gsd_background_helper_CFLAGS =		\
		`pkg-config --cflags gtk+-3.0`

gsd_background_helper_LDFLAGS=		\
		`pkg-config --libs gtk+-3.0` \
		-lm


EXTRA_DIST = 			\
	$(plugin_in_files)	\
	$(NULL)

CLEANFILES = 			\
	$(plugin_DATA)		\
	$(NULL)

DISTCLEANFILES =		\
	$(plugin_DATA)		\
	$(NULL)

@GSD_INTLTOOL_PLUGIN_RULE@
