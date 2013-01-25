


#ifndef _GSD_KEY_BINDINGS_UTIL_H_
#define _GSD_KEY_BINDINGS_UTIL_H_

#include <gio/gio.h>
#include <glib.h>

#include "gsd-key-bindings-handler.h"

typedef struct _KeysAndCmd	KeysAndCmd;

struct _KeysAndCmd
{
    char* keystring;
    char* cmdstring;
};

void		gsd_kb_util_read_gsettings		(GSettings* settings,		
							 GHashTable* gsettings_ht); //output variable.
KeysAndCmd*	gsd_kb_util_parse_gsettings_value	(char* gsettings_key,
							 char* string);

void		gsd_kb_util_key_free_func		(gpointer key);
void		gsd_kb_util_value_free_func		(gpointer value);


#endif
