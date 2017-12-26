/* @author page
 * @date 2017/12/26
 */

 #ifndef _RDFS_SERVER_H
 #define _RDFS_SERVER_H

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

 #define IP_MAX_LENGTH  256
 #define NETWORK_BUFF_LENGTH 1024
struct rdfs_message
{
    int type;
    char data[NETWORK_BUFF_LENGTH];
};
enum RDFS_CLIENT_REQUEST
{
    RDFS_CLIENT_OPEN,
    RDFS_CLIENT_CLOSE,
    RDFS_CLIENT_MKDIR,
    RDFS_CLIENT_RMDIR,
    RDFS_CLIENT_LISTDIR
};
int rdfs_init_listen_socket(int e_fd,const char*ip,int port);
void rdfs_accept_connection(int fd, int events, void *arg);
void rdfs_recv_data(int fd, int events, void *arg);
void rdfs_send_data(int fd, int events, void *arg);
int rdfs_init_network(const char* ip,int port);
 #endif