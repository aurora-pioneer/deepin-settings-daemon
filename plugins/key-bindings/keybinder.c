#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include "keybinder.h"
#include "parse-super.h"
#include "gsd-keygrab.h"

struct Binding {
    KeybinderHandler      handler;
    void                 *user_data;
    char                 *keystring;
    GDestroyNotify        notify;
};

static void init_grab_key_xi2_manager (void);
static gboolean init_screen_list (void);
static GdkFilterReturn filter_key_events (XEvent *xevent,
        GdkEvent *event, gpointer user_data);
static void keymap_changed (GdkKeymap *map);
static gboolean do_grab_key (struct Binding *binding);
static gboolean do_ungrab_key (struct Binding *binding);

GHashTable *key_table;
GSList *screens;
static GSList *bindings = NULL;

/**
 * keybinder_init:
 *
 * Initialize the keybinder library.
 *
 * This function must be called after initializing GTK, before calling any
 * other function in the library. Can only be called once.
 */
void
keybinder_init ()
{
    GdkKeymap *keymap = gdk_keymap_get_default ();
    Display *disp;

    if (!(disp = XOpenDisplay(NULL))) {
        g_warning("keybinder_init: Unable to open display");
        return;
    }

    /* Workaround: Make sure modmap is up to date
     * There is possibly a bug in GTK+ where virtual modifiers are not
     * mapped because the modmap is not updated. The following function
     * updates it.
     */
    (void) gdk_keymap_have_bidi_layouts(keymap);


    g_signal_connect (keymap,
                      "keys_changed",
                      G_CALLBACK (keymap_changed),
                      NULL);

    /*
     * XRecord
     * parse super
     */
    key_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free, g_free);

    if ( !init_xrecord () ) {
        g_debug ("init xrecord failed!");
    }

    init_grab_key_xi2_manager ();
}

static void
keymap_changed (GdkKeymap *map)
{
    GSList *iter;

    (void) map;

    g_debug ("Keymap changed! Regrabbing keys...");

    for (iter = bindings; iter != NULL; iter = iter->next) {
        struct Binding *binding = iter->data;
        do_ungrab_key (binding);
    }

    for (iter = bindings; iter != NULL; iter = iter->next) {
        struct Binding *binding = iter->data;
        do_grab_key (binding);
    }
}


static gboolean
do_grab_key (struct Binding *binding)
{
    if ( g_strcmp0 (binding->keystring, "Super") == 0 ) {
        g_debug ("bind key Super!\n");
        insert_table_record (0xffeb, binding->user_data);
        insert_table_record (0xffec, binding->user_data);
        return TRUE;
    }

    Key *key = parse_key (binding->keystring);

    if ( key == NULL ) {
        return FALSE;
    }

    grab_key_unsafe (key, TRUE, GSD_KEYGRAB_SYNCHRONOUS, screens);
    free_key (key);

    return TRUE;
}

static gboolean
do_ungrab_key (struct Binding *binding)
{
    if ( g_strcmp0 (binding->keystring, "Super") == 0 ) {
        remove_table_record (0xffeb);
        remove_table_record (0xffec);
        return TRUE;
    }

    Key *key = parse_key (binding->keystring);

    if ( key == NULL ) {
        return FALSE;
    }

    grab_key_unsafe (key, FALSE, GSD_KEYGRAB_SYNCHRONOUS, screens);
    free_key (key);

    return TRUE;
}

/**
 * keybinder_bind: (skip)
 * @keystring: an accelerator description (gtk_accelerator_parse() format)
 * @handler:   callback function
 * @user_data: data to pass to @handler
 *
 * Grab a key combination globally and register a callback to be called each
 * time the key combination is pressed.
 *
 * This function is excluded from introspected bindings and is replaced by
 * keybinder_bind_full.
 *
 * Returns: %TRUE if the accelerator could be grabbed
 */
gboolean
keybinder_bind (const char *keystring,
                KeybinderHandler handler,
                void *user_data)
{
    return keybinder_bind_full(keystring, handler, user_data, NULL);
}

/**
 * keybinder_bind_full:
 * @keystring: an accelerator description (gtk_accelerator_parse() format)
 * @handler:   (scope notified):        callback function
 * @user_data: (closure) (allow-none):  data to pass to @handler
 * @notify:    (allow-none):  called when @handler is unregistered
 *
 * Grab a key combination globally and register a callback to be called each
 * time the key combination is pressed.
 *
 * Rename to: keybinder_bind
 *
 * Since: 0.3.0
 *
 * Returns: %TRUE if the accelerator could be grabbed
 */
gboolean
keybinder_bind_full (const char *keystring,
                     KeybinderHandler handler,
                     void *user_data,
                     GDestroyNotify notify)
{
    struct Binding *binding = NULL;
    gboolean success = FALSE;

    binding = g_new0 (struct Binding, 1);

    if ( binding == NULL ) {
        return success;
    }

    binding->keystring = g_strdup (keystring);
    binding->handler = handler;
    binding->user_data = user_data;
    binding->notify = notify;

    /* Sets the binding's keycode and modifiers */
    success = do_grab_key (binding);

    if (success) {
        g_debug ("keybinder_bind_full: do_grab_key success");
        bindings = g_slist_prepend (bindings, binding);
    } else {
        g_debug ("keybinder_bind_full: do_grab_key failed");
        g_free (binding->keystring);
        g_free (binding);
    }

    return success;
}

/**
 * keybinder_unbind: (skip)
 * @keystring: an accelerator description (gtk_accelerator_parse() format)
 * @handler:   callback function
 *
 * Unregister a previously bound callback for this keystring.
 *
 * NOTE: multiple callbacks per keystring are not properly supported. You
 * might as well use keybinder_unbind_all().
 *
 * This function is excluded from introspected bindings and is replaced by
 * keybinder_unbind_all().
 */
void
keybinder_unbind (const char *keystring, KeybinderHandler handler)
{
    GSList *iter;

    for (iter = bindings; iter != NULL; iter = iter->next) {
        struct Binding *binding = iter->data;

        if (strcmp (keystring, binding->keystring) != 0 ||
                handler != binding->handler) {
            continue;
        }

        do_ungrab_key (binding);
        bindings = g_slist_remove (bindings, binding);

        g_debug ("unbind, notify: %p", binding->notify);

        if (binding->notify) {
            binding->notify(binding->user_data);
        }

        g_free (binding->keystring);
        g_free (binding);
        break;
    }
}

/**
 * keybinder_unbind_all:
 * @keystring: an accelerator description (gtk_accelerator_parse() format)
 *
 * Unregister all previously bound callbacks for this keystring.
 *
 * Rename to: keybinder_unbind
 *
 * Since: 0.3.0
 */
void keybinder_unbind_all (const char *keystring)
{
    GSList *iter = bindings;

    for (iter = bindings; iter != NULL; iter = iter->next) {
        struct Binding *binding = iter->data;

        if (strcmp (keystring, binding->keystring) != 0) {
            continue;
        }

        do_ungrab_key (binding);
        bindings = g_slist_remove (bindings, binding);

        g_debug ("unbind_all, notify: %p", binding->notify);

        if (binding->notify) {
            binding->notify(binding->user_data);
        }

        g_free (binding->keystring);
        g_free (binding);

        /* re-start scan from head of new list */
        iter = bindings;

        if (!iter) {
            break;
        }
    }
}

static void
init_grab_key_xi2_manager (void)
{
    if ( !init_screen_list () ) {
        return;
    }

    GSList *l;

    for ( l = screens; l != NULL; l = l->next ) {
        gdk_window_add_filter (gdk_screen_get_root_window (l->data),
                               (GdkFilterFunc) filter_key_events, NULL);
    }
}

void
destroy_grab_xi2_manager (void)
{
    GSList *l;

    for ( l = screens; l != NULL; l = l->next ) {
        gdk_window_remove_filter (gdk_screen_get_root_window (l->data),
                                  (GdkFilterFunc) filter_key_events, NULL);
    }

    g_slist_free (screens);
    return;
}

static gboolean
init_screen_list (void)
{
    GdkDisplay *display;

    display = gdk_display_get_default ();

    if ( display == NULL ) {
        g_warning ("Get Default Display Failed");
        return FALSE;
    }

    int len = gdk_display_get_n_screens (display);
    int i;

    for ( i = 0; i < len; i++ ) {
        GdkScreen *screen;

        screen = gdk_display_get_screen (display, i);

        if ( screen == NULL || !GDK_IS_SCREEN (screen) ) {
            g_warning ("get screen failed");
            continue;
        }

        screens = g_slist_append (screens, screen);
    }

    return TRUE;
}

static GdkFilterReturn
filter_key_events (XEvent *xevent, GdkEvent *event, gpointer user_data)
{
    XIEvent	*xiev;
    XIDeviceEvent	*xev;

    // verify we have a key event
    if ( xevent->type != GenericEvent ) {
        return GDK_FILTER_CONTINUE;
    }

    xiev = (XIEvent *) xevent->xcookie.data;

    if ( xiev->evtype != XI_KeyPress && xiev->evtype != XI_KeyRelease ) {
        return GDK_FILTER_CONTINUE;
    }

    xev = (XIDeviceEvent *)xiev;

    if ( xiev->evtype == XI_KeyRelease ) {
        GSList *iter = bindings;

        while (iter != NULL) {
            struct Binding *binding = iter->data;
            iter = iter->next;

            Key *key = parse_key (binding->keystring);
            gboolean is_equal = match_xi2_key (key, xev);
            free_key (key);

            if ( is_equal ) {
                GdkWindow *window = gdk_get_default_root_window ();

                if ( GDK_WINDOW_XID(window) == xev->root) {
                    (binding->handler) (binding->keystring,
                                        binding->user_data);
                    return GDK_FILTER_CONTINUE;
                }
            }
        }
    }

    return GDK_FILTER_CONTINUE;
}
