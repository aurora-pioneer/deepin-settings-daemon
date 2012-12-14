#include <glib.h>
#include <sys/inotify.h>

static int m_inotifyfd = -1;

static void m_inotify_events_io_cb(struct inotify_event *event, gpointer data) 
{
    printf("DEBUG m_inotify_events_io_cb\n");
}

int main(int argc, char **argv) 
{
    int wd = -1;
    GIOChannel *channel = NULL;

    m_inotifyfd = inotify_init();
    if (-1 == m_inotifyfd) {
        printf("fail to inotify_init\n");
        return -1;
    }

    wd = inotify_add_watch(m_inotifyfd, "/home/zhaixiang/.config/monitors.xml", IN_MODIFY);
    if (-1 == wd) {
        printf("fail to inotify_add_watch\n");
        return -1;
    }
    
    channel = g_io_channel_unix_new(m_inotifyfd);
    g_io_add_watch(channel, G_IO_IN, m_inotify_events_io_cb, NULL);
    
    GMainLoop* loop = g_main_loop_new(NULL, FALSE);
    
    g_main_loop_run(loop);
    
    if (m_inotifyfd) {
        inotify_rm_watch(m_inotifyfd, wd);
        close(m_inotifyfd);
        m_inotifyfd = -1;
    }

    return 0;
}
