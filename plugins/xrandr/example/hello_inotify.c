#include <stdio.h>
#include <assert.h>
#include <sys/inotify.h>
#include <sys/epoll.h>

#define EPOLL_SIZE 3

int main(int argc, char **argv)
{
    int inotifyfd = -1;
    int wd = -1;
    int epollfd = -1;
    int nfds;
    int i;
    struct epoll_event ev;
    struct epoll_event events[EPOLL_SIZE];

    /*creating the INOTIFY instance*/
    inotifyfd = inotify_init();

    /*checking for error*/
    assert(-1 != inotifyfd);

    wd = inotify_add_watch(inotifyfd, 
                           "/home/zhaixiang/.config/monitors.xml", 
                           IN_MODIFY);

    epollfd = epoll_create(EPOLL_SIZE);
    assert(-1 != epollfd);
    ev.events = EPOLLIN;
    ev.data.fd = inotifyfd;
    assert(0 == epoll_ctl(epollfd, EPOLL_CTL_ADD, inotifyfd, &ev));
    
    while (1) {
        nfds = epoll_wait(epollfd, events, EPOLL_SIZE, -1);
        for (i = 0; i < nfds; ++i) {
            if (inotifyfd == events[i].data.fd) {
                printf("DEBUG inotify\n");
            }
        }
        usleep(100);
    }

    inotify_rm_watch(inotifyfd, wd);
    wd = -1;

    /*closing the INOTIFY instance*/
    close(inotifyfd);
    inotifyfd = -1;
}
