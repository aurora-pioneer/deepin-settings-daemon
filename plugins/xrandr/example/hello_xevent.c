#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/extensions/randr.h>
#include <X11/extensions/Xrandr.h>

static Display *m_display = NULL;
static Window m_window, m_root;

int main(int argc, char **argv) 
{
    XEvent ev;
    XSetWindowAttributes attrib;
    int major_version_return;
    int minor_version_return;
    int event_base_return;
    int error_base_return;

    /* NULL defaults to the value of the DISPLAY environment variable */
    m_display= XOpenDisplay(NULL);
    if (!m_display) {
        printf("fail to XOpenDisplay\n");
        return -1;
    }
    
    XRRQueryVersion(m_display, &major_version_return, &minor_version_return);
    XRRQueryExtension(m_display, &event_base_return, &error_base_return);
    printf("DEBUG %d %d %d %d\n", 
        major_version_return, minor_version_return, event_base_return, error_base_return);
    /*attrib.override_redirect= True;*/
    m_window = XCreateWindow(m_display, 
                             DefaultRootWindow(m_display), 
                             0, 
                             0, 
                             400, 
                             300, 
                             10, 
                             CopyFromParent, 
                             InputOutput, 
                             CopyFromParent, 
                             0/*CWOverrideRedirect*/, 
                             &attrib);
    
    XSelectInput(m_display, m_window, KeyPressMask | KeyReleaseMask);
    XRRSelectInput(m_display, m_window, RRScreenChangeNotifyMask |
        RROutputChangeNotifyMask | RRCrtcChangeNotifyMask | RROutputPropertyNotifyMask);

    XMapWindow(m_display, m_window);

    for (;;) {
        XNextEvent(m_display, &ev);
        printf("DEBUG event type %d\n", ev.type);
        //usleep(100);
    }

    return 0;
}
