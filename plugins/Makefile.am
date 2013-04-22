NULL =

enabled_plugins =	\
	a11y-settings	\
	a11y-keyboard	\
	automount	\
	background	\
	clipboard	\
	cursor		\
	datetime	\
	color		\
	dummy		\
	keyboard	\
	power		\
	housekeeping    \
	idledelay	\
	media-keys	\
	mouse		\
	sound		\
	wacom		\
	xrandr		\
	xsettings	\
	key-bindings	\
    $(NULL)

disabled_plugins = \
	$(NULL)

if BUILD_GCONF_BRIDGE
enabled_plugins += gconf
else
disabled_plugins += gconf
endif

if HAVE_PACKAGEKIT
enabled_plugins += updates
else
disabled_plugins += updates
endif

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
