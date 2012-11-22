plugin_name = helloworld

plugin_LTLIBRARIES =					\
	libhelloworld.la

libhelloworld_la_SOURCES = 					\
	gsd-helloworld-manager.c				\
	gsd-helloworld-manager.h				\
	gsd-helloworld-plugin.c				\
	gsd-helloworld-plugin.h

libhelloworld_la_CPPFLAGS = \
	-I$(top_srcdir)/data/				\
	-I$(top_srcdir)/gnome-settings-daemon		\
	-I$(top_srcdir)/plugins/common			\
	-DGNOME_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"		\
	-DSBINDIR=\"$(sbindir)\"			\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	$(AM_CPPFLAGS)

libhelloworld_la_CFLAGS = \
	$(PLUGIN_CFLAGS)				\
	$(KEYRING_CFLAGS)				\
	$(GNOME_DESKTOP_CFLAGS)				\
	$(SETTINGS_PLUGIN_CFLAGS)			\
	$(AM_CFLAGS)

libhelloworld_la_LDFLAGS = 					\
	$(GSD_PLUGIN_LDFLAGS)

libhelloworld_la_LIBADD  = 					\
	$(top_builddir)/plugins/common/libcommon.la	\
	$(top_builddir)/gnome-settings-daemon/libgsd.la \
	$(GNOME_DESKTOP_LIBS)				\
	$(KEYRING_LIBS)					\
	$(SETTINGS_PLUGIN_LIBS)

plugin_in_files = 					\
	helloworld.gnome-settings-plugin.in

plugin_DATA = $(plugin_in_files:.gnome-settings-plugin.in=.gnome-settings-plugin)

com.deepin.settings-daemon.plugins.helloworld.policy.in: com.deepin.settings-daemon.plugins.helloworld.policy.in.in Makefile
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

@INTLTOOL_POLICY_RULE@
polkit_policydir = $(datadir)/polkit-1/actions
polkit_policy_in_files = com.deepin.settings-daemon.plugins.helloworld.policy.in
polkit_policy_DATA = $(polkit_policy_in_files:.policy.in=.policy)

EXTRA_DIST = 						\
	com.deepin.settings-daemon.plugins.helloworld.policy.in.in

clean-local:
	rm -f *~

CLEANFILES = 						\
	$(plugin_DATA)					\
	com.deepin.settings-daemon.plugins.helloworld.policy	\
	com.deepin.settings-daemon.plugins.helloworld.policy.in

@GSD_INTLTOOL_PLUGIN_RULE@
