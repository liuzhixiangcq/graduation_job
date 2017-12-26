/* @author page
 * @date 2017/12/26
 */

 #include <stdio.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <dirent.h>
 #include <string.h>
 #include <netinet/in.h>
 #include <stdlib.h>
 #include <errno.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <sys/stat.h>
 #include <sys/time.h>
 #include <sys/epoll.h>
 #include <sys/vfs.h>
 #include <sys/statvfs.h>
 
 #include "rdfs_config.h"
 #include "rdfs_server.h"
 #include "event.h"

    
void rdfs_event_set(struct rdfs_event *ev, int fd, void (*call_back)(int, int, void*), void *arg)    
{    
    ev->fd = fd;    
    ev->call_back = call_back;    
    ev->events = 0;    
    ev->arg = ev;    
    ev->status = 0;  
    if(arg == NULL)
    {
        memset(ev->message.data,0,sizeof(ev->message.data));
        //bzero(ev->buff, sizeof(ev->buff));  
    }       
    printf("%s \n",__func__);
}    
// add/mod an event to epoll    
void rdfs_event_add(int epoll_fd, int events, struct rdfs_event *ev)    
{    
    struct epoll_event epv = {0, {0}};    
    int op;    
    epv.data.ptr = ev;    
    epv.events = ev->events = events;    
    if(ev->status == 1)
    {    
        op = EPOLL_CTL_MOD;    
    }    
    else
    {    
        op = EPOLL_CTL_ADD;    
        ev->status = 1;    
    }    
    printf("epoll_fd:%d op:%d,fd:%d,data:%lx\n",epoll_fd,op,ev->fd,&epv);
    int ret = epoll_ctl(epoll_fd, op, ev->fd, &epv);
    if(ret < 0)    
        printf("Event Add failed[fd=%d], evnets[%d] ret:%d errno:%d %s\n", ev->fd, events,ret,errno,strerror(errno));    
    else    
        printf("Event Add OK[fd=%d], op=%d, evnets[%0X]\n", ev->fd, op, events);    
}    
// delete an event from epoll    
void rdfs_event_del(int epoll_fd, struct rdfs_event *ev)    
{    
    struct epoll_event epv = {0, {0}};    
    if(ev->status != 1) return;    
    epv.data.ptr = ev;    
    ev->status = 0;  
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ev->fd, &epv) < 0)    
        printf("Event Del failed[fd=%d]\n", ev->fd);    
    else    
        printf("Event Del OK[fd=%d]\n", ev->fd);  
       
}