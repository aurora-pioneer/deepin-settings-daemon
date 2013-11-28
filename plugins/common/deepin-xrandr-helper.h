#ifndef __DEEPIN_XRANDR_HELPER__
#define __DEEPIN_XRANDR_HELPER__

#include <glib.h>
void deepin_xrandr_set_brightness(double value, gboolean effect_gsetting);
double deepin_xrandr_get_brightness(void);

#endif
