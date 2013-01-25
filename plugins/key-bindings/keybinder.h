#ifndef __KEY_BINDER_H__
#define __KEY_BINDER_H__

#include <glib.h>

G_BEGIN_DECLS

typedef void (* KeybinderHandler) (const char *keystring, void *user_data);

void keybinder_init (void);

void keybinder_set_use_cooked_accelerators (gboolean use_cooked);

gboolean keybinder_bind (const char *keystring,
                         KeybinderHandler  handler,
                         void *user_data);

gboolean
keybinder_bind_full (const char *keystring,
                     KeybinderHandler handler,
                     void *user_data,
                     GDestroyNotify notify);

void keybinder_unbind (const char *keystring,
                       KeybinderHandler  handler);

void keybinder_unbind_all (const char *keystring);

guint32 keybinder_get_current_event_time (void);

G_END_DECLS

#endif /* __KEY_BINDER_H__ */

