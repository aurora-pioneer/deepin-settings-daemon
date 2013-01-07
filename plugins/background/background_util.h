#ifndef _DEEPIN_BACKGROUND_H_
#define _DEEPIN_BACKGROUND_H_

#include <gio/gio.h>
#include "gsd-background-manager.h"
#include "gsd-background-manager-private.h"

#define DEEPIN_EXPORT

// all schema related information.
#define	BG_SCHEMA_ID	"com.deepin.dde.background"

#define BG_FPS		30		//in my implementation, the actual fps may not be 30.

#define	BG_CURRENT_PICT "current-picture"
#define BG_PICTURE_URIS	"picture-uris" //better renamed to picture-URIs
#define DELIMITER	';'		//picture-uri string delimiter 
#define BG_BG_DURATION	"background-duration"
#define BG_XFADE_MANUAL_INTERVAL "cross-fade-manual-interval"     //manually change background
#define BG_XFADE_AUTO_INTERVAL	 "cross-fade-auto-interval"       //automatically change background

//exporting functions for gsd-background-manager.c
extern void bg_util_init (GsdBackgroundManager* manager);

extern void bg_util_connect_screen_signals (GsdBackgroundManager* manager);
extern void bg_util_disconnect_screen_signals (GsdBackgroundManager* manager);

#endif
