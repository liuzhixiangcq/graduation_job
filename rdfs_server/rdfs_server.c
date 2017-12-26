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
 #include <fcntl.h>
 #include "rdfs_config.h"
 #include "rdfs_server.h"
 #include "event.h"
 
 int global_epoll_fd = 0;
 struct rdfs_event g_events[MAX_EPOLL_EVENTS];

 int rdfs_init_listen_socket(int e_fd,const char*ip,int port)
 {
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);    
    fcntl(listenFd, F_SETFL, O_NONBLOCK);   

    printf("server listen fd=%d\n", listenFd);    
    struct sockaddr_in sin;    
    bzero(&sin, sizeof(sin));    
    sin.sin_family = AF_INET;    
    sin.sin_addr.s_addr = inet_addr(ip);    
    sin.sin_port = htons(port);    

    bind(listenFd, (struct sockaddr*)&sin, sizeof(sin));    
    listen(listenFd, SERVER_SOCKET_LISTEN_NUMS); 
    
    rdfs_event_set(&g_events[MAX_EPOLL_EVENTS-1], listenFd, rdfs_accept_connection, NULL);   
    //printf("e_fd:%d,epoll_fd:%d\n",e_fd,epoll_fd); 
    rdfs_event_add(e_fd, EPOLLIN, &g_events[MAX_EPOLL_EVENTS-1]);  
    return 0;
 }
 void rdfs_accept_connection(int fd, int events, void *arg)    
 {    
     printf("%s\n",__func__);
     struct sockaddr_in sin;    
     socklen_t len = sizeof(struct sockaddr_in);    
     int nfd, i;     
     if((nfd = accept(fd, (struct sockaddr*)&sin, &len)) == -1)    
     {    
         printf("%s: accept, %d", __func__, errno);    
         return;    
     }    

    for(i = 0; i < MAX_EPOLL_EVENTS; i++)    
    {    
        if(g_events[i].status == 0)    
        {    
            break;    
        }    
    }    
    if(i == MAX_EPOLL_EVENTS)    
    {    
        printf("%s:max connection limit[%d].", __func__, MAX_EPOLL_EVENTS);     
    }    
    // set nonblocking  
    int iret = 0;  
    if((iret = fcntl(nfd, F_SETFL, O_NONBLOCK)) < 0)  
    {  
        printf("%s: fcntl nonblocking failed:%d", __func__, iret);  
    }  
    // add a read event for receive data    
    rdfs_event_set(&g_events[i], nfd, rdfs_recv_data, NULL);    
    rdfs_event_add(global_epoll_fd, EPOLLIN, &g_events[i]);       
    printf("new conn[%s:%d],pos[%d]\n", inet_ntoa(sin.sin_addr),ntohs(sin.sin_port),i);    
 }    
int rdfs_process_request(struct rdfs_event *ev)
{
    
}
 // receive data    
void rdfs_recv_data(int fd, int events, void *arg)    
{    
    printf("%s\n",__func__);
    struct rdfs_event *ev = (struct rdfs_event*)arg;    
    int len;    
    // receive data  
    len = recv(fd, &ev->message, sizeof(struct rdfs_message), 0);  
    rdfs_process_request(ev);    
    rdfs_event_del(global_epoll_fd, ev);  
    if(len > 0)  
    {    
        ev->message.data[len] = '\0';    
        printf("C[%d]:%s\n", fd, ev->message.data);    
        // change to send event    
        rdfs_event_set(ev, fd, rdfs_send_data, ev);    
        rdfs_event_add(global_epoll_fd, EPOLLOUT, ev);    
    }    
    else if(len == 0)    
    {    
        close(ev->fd);    
        printf("[fd=%d] pos[%d], closed gracefully.\n", fd, ev-g_events);    
    }    
    else    
    {    
        close(ev->fd);    
        printf("recv[fd=%d] error[%d]:%s\n", fd, errno, strerror(errno));    
    }    
}    
// send data    
void rdfs_send_data(int fd, int events, void *arg)    
{    
    printf("%s \n",__func__);
    struct rdfs_event *ev = (struct rdfs_event*)arg;    
    int len;    
    // send data    
   // len = send(fd,"hello world",20);
    ev->message.type = 0;
    len = send(fd, &ev->message,sizeof(struct rdfs_message) , 0);  
    if(len > 0)    
    {  
        printf("send[fd=%d], [%d] %s\n", fd, len, ev->message.data);  
 
        rdfs_event_del(global_epoll_fd, ev);    
        rdfs_event_set(ev, fd, rdfs_recv_data, NULL);    
        rdfs_event_add(global_epoll_fd, EPOLLIN, ev);    
    }    
    else    
    {    
        close(ev->fd);    
        rdfs_event_del(global_epoll_fd, ev);    
        printf("send[fd=%d] error[%d]\n", fd, errno);    
    }    
}    
 int rdfs_init_network(const char* ip,int port)
 {
    int epoll_fd;
    epoll_fd = epoll_create(MAX_EPOLL_EVENTS);
    printf("%s epoll_fd=%d\n",__func__,epoll_fd);
    if(epoll_fd <= 0)
    {
        printf("epoll creat fd=%d\n",epoll_fd);
        return -1;
    }
    int ret;
    ret = rdfs_init_listen_socket(epoll_fd,ip,port);
    global_epoll_fd = epoll_fd;
    struct epoll_event rdfs_events[MAX_EPOLL_EVENTS];
    int check_pos = 0;
    while(1)
    {    
       // printf("epoll_fd:%d global_epoll_fd:%d\n",epoll_fd,global_epoll_fd);
        int fds = epoll_wait(epoll_fd, rdfs_events, MAX_EPOLL_EVENTS, 1000); 
       // printf("epoll wait fds:%d\n",fds);   
        if(fds < 0){    
            printf("epoll_wait error, exit\n");    
            break;    
        }    
        for(int i = 0; i < fds; i++)
        {    
            struct rdfs_event *ev = (struct rdfs_event*)rdfs_events[i].data.ptr;    
            if((rdfs_events[i].events & EPOLLIN) && (ev->events & EPOLLIN))  
            {    
                //printf("epoll in fd:%d events:%d args:%lx\n",ev->fd, rdfs_events[i].events,ev->arg);
                ev->call_back(ev->fd, rdfs_events[i].events, ev->arg);    
            }    
            if((rdfs_events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT))   
            {    
              //  printf("epoll out\n");
                ev->call_back(ev->fd, rdfs_events[i].events, ev->arg);    
            }    
        }    
    }
    return 0;
 }