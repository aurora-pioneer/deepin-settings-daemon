NULL =

plugin_name = key-bindings

if BUILD_TEST_PROGRAMS
noinst_PROGRAMS =			\
	test-key-bindings		\
	test-keybinder			\
	$(NULL)
test_keybinder_SOURCES =		\
	test-keybinder.c		\
	keybinder.c			\
	keybinder.h			\
	$(NULL)

test_keybinder_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"		\
	-DLIBEXECDIR=\""$(libexecdir)"\"			\
	$(AM_CPPFLAGS)

test_keybinder_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS) -g

test_keybinder_LDADD =		\
	$(top_builddir)/gnome-settings-daemon/libgsd.la	\
	$(top_builddir)/plugins/common/libcommon.la			\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(NULL)

test_key_bindings_SOURCES =		\
	test-key-bindings.c		\
	gsd-key-bindings-handler.c	\
	gsd-key-bindings-handler.h	\
	gsd-key-bindings-manager.c	\
	gsd-key-bindings-manager.h	\
	gsd-key-bindings-settings.h	\
	gsd-key-bindings-util.c		\
	gsd-key-bindings-util.h		\
	keybinder.c			\
	keybinder.h			\
	parse-super.h			\
	parse-super.c			\
	gsd-key-bindings-custom.h			\
	gsd-key-bindings-custom.c			\
	$(NULL)

test_key_bindings_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"		\
	-DLIBEXECDIR=\""$(libexecdir)"\"			\
	$(AM_CPPFLAGS)

test_key_bindings_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(KEY_BINDINGS_CFLAGS)		\
	$(AM_CFLAGS) -g

test_key_bindings_LDADD =		\
	$(top_builddir)/gnome-settings-daemon/libgsd.la	\
	$(top_builddir)/plugins/common/libcommon.la			\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(KEY_BINDINGS_LIBS)	\
	$(NULL)
endif

plugin_LTLIBRARIES = \
	libkey-bindings.la

libkey_bindings_la_SOURCES = 		\
	gsd-key-bindings-handler.c	\
	gsd-key-bindings-handler.h	\
	gsd-key-bindings-manager.c	\
	gsd-key-bindings-manager.h	\
	gsd-key-bindings-plugin.c	\
	gsd-key-bindings-settings.h	\
	gsd-key-bindings-util.c		\
	gsd-key-bindings-util.h		\
	keybinder.c			\
	keybinder.h			\
	parse-super.h			\
	parse-super.c			\
	gsd-key-bindings-custom.h			\
	gsd-key-bindings-custom.c			\
	$(NULL)

libkey_bindings_la_CPPFLAGS = \
	-I$(top_srcdir)/gnome-settings-daemon		\
	-I$(top_srcdir)/plugins/common				\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libkey_bindings_la_CFLAGS = \
	$(PLUGIN_CFLAGS)		\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libkey_bindings_la_LDFLAGS = -lX11 -lXtst		\
	$(GSD_PLUGIN_LDFLAGS)

libkey_bindings_la_LIBADD  = 		\
	$(top_builddir)/plugins/common/libcommon.la			\
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
