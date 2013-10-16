#define NEW gsd_keyboard_manager_new
#define START gsd_keyboard_manager_start
#define STOP gsd_keyboard_manager_stop
#define MANAGER GsdKeyboardManager

#include "config.h"

#include <stdlib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gsd-keyboard-manager.h"

/*#include "test-plugin.h"*/

int main (int argc, char *argv[])
{
    GError  *error;
    GsdKeyboardManager *manager = NULL;

    bindtextdomain (GETTEXT_PACKAGE, GNOME_SETTINGS_LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    error = NULL;
    if (! gtk_init_with_args (&argc, &argv, NULL, NULL, NULL, &error)) {
        fprintf (stderr, "%s", error->message);
        g_error_free (error);
        exit (1);
    }

    error = NULL;
    manager = gsd_keyboard_manager_new ();

    g_print ("Test!!!\n");
    gsd_keyboard_manager_start (manager, &error);

    gtk_main ();

    return 0;
}
