#ifndef __KEY_BINDER_H__
#define __KEY_BINDER_H__

#include <glib.h>

G_BEGIN_DECLS

typedef void (* KeybinderHandler) (const char *keystring, void *user_data);

void keybinder_init (void);

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

void destroy_grab_xi2_manager (void);

G_END_DECLS

#endif /* __KEY_BINDER_H__ */

