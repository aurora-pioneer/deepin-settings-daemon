#include <stdio.h>
#include <X11/Xlib.h>

static Display *m_display = NULL;
static Window m_window, m_root;

int main(int argc, char **argv) 
{
    XEvent ev;
    XSetWindowAttributes attrib;

    /* NULL defaults to the value of the DISPLAY environment variable */
    m_display= XOpenDisplay(NULL);
    if (!m_display) {
        printf("fail to XOpenDisplay\n");
        return -1;
    }

    attrib.override_redirect= True;
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
                             CWOverrideRedirect, 
                             &attrib);
    
    XMapWindow(m_display, m_window);

    /*
    if (GrabSuccess != XGrabKeyboard(m_display, m_window, False, GrabModeAsync, GrabModeAsync, CurrentTime)) {
        printf("fail to XGrabKeyboard\n");
        return -1;
    }
    */

    for (;;) {
        XNextEvent(m_display, &ev);
        printf("DEBUG event type %d\n", ev.type);
        switch (ev.type) {
            
        }
    }

    
    return 0;
}
