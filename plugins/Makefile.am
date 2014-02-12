NULL =

enabled_plugins =	\
	a11y-keyboard	\
	a11y-settings	\
	clipboard	\
	datetime	\
	dummy		\
	idle-delay	\
	keyboard	\
	media-keys	\
	mouse		\
	power		\
	print-notifications\
	xsettings	\
	$(NULL)

disabled_plugins = \
	$(NULL)

if BUILD_PRINT_NOTIFICATIONS
enabled_plugins += print-notifications
else
disabled_plugins += print-notifications
endif

SUBDIRS = common $(enabled_plugins)
DIST_SUBDIRS = $(SUBDIRS) $(disabled_plugins)
