#include "config.h"

#include <stdlib.h>
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gsd-idle-delay-manager.h"
#include "gsd-idle-delay-plugin.h"

static gboolean
idle (GsdIdleDelayManager* manager)
{
        gsd_idle_delay_manager_start (manager, NULL);
        return FALSE;
}

int
main (int argc, char *argv[])
{
        GsdIdleDelayManager* manager;

        bindtextdomain (GETTEXT_PACKAGE, GNOME_SETTINGS_LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

	g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);

        gtk_init (&argc, &argv);

        manager = gsd_idle_delay_manager_new ();
        g_idle_add ((GSourceFunc)idle, manager);

        gtk_main ();

        return 0;
}
