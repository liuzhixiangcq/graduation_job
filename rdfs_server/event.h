/* @author page
 * @date 2017/12/26
 */

 #ifndef _RDFS_EVENT_H
 #define _RDFS_EVENT_H
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

 #define NETWORK_BUFF_LENGTH 1024
 struct rdfs_event    
 {    
     int fd;    
     void (*call_back)(int fd, int events, void *arg);    
     int events;    
     void *arg;    
     int status; // 1: in epoll wait list, 0 not in   
     struct rdfs_message message; 
     //char buff[NETWORK_BUFF_LENGTH];    
    // int len, s_offset;    
     //long last_active;
 }; 
void rdfs_event_set(struct rdfs_event *ev, int fd, void (*call_back)(int, int, void*), void *arg);
void rdfs_event_add(int epollFd, int events, struct rdfs_event *ev) ;
void rdfs_event_del(int epoll_fd, struct rdfs_event *ev) ;
 #endif