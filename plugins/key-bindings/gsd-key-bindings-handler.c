#include <glib.h>

#include "gsd-key-bindings-handler.h"
#include "gsd-key-bindings-util.h"
#include "keybinder.h"


void 
gsd_kb_handler_default	(const char* keystring, void* user_data)
{
	char* _cmd_str = (char *) user_data; 
	g_debug ("Invoking default key binding(%s) handler with command: %s",keystring, _cmd_str);

	GError* error = NULL;
	g_spawn_command_line_async (_cmd_str, &error);
	if (error != NULL)
	{
	    g_warning ("Invoking %s error: %s", _cmd_str, error->message);
	    g_error_free (error);
	}
}
