NULL =

enabled_plugins =	\
	a11y-keyboard	\
	a11y-settings	\
	background	\
	clipboard	\
	cursor		\
	datetime	\
	dummy		\
	housekeeping    \
	idle-delay	\
	keyboard	\
	media-keys	\
	mouse		\
	orientation	\
	power		\
	print-notifications\
	sound		\
	wacom		\
	xrandr		\
	xsettings	\
	$(NULL)

disabled_plugins = \
	color	\
	$(NULL)

if SMARTCARD_SUPPORT
enabled_plugins += smartcard
else
disabled_plugins += smartcard
endif

if HAVE_GUDEV
enabled_plugins += orientation
else
disabled_plugins += orientation
endif

if BUILD_PRINT_NOTIFICATIONS
enabled_plugins += print-notifications
else
disabled_plugins += print-notifications
endif

SUBDIRS = common $(enabled_plugins)
DIST_SUBDIRS = $(SUBDIRS) $(disabled_plugins)
