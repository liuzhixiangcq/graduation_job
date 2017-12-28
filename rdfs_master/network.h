/* @author page
 * @date 2017/12/5
 * rdfs_master/network.h
 */

 #ifndef _RDFS_NETWORK_H
 #define _RDFS_NETWORK_H

 #include <linux/kernel.h>
 #include <linux/init.h>
 #include <linux/un.h>
 #include <linux/net.h>
 #include <net/sock.h>
 #include <linux/socket.h>
 #include <linux/module.h>
 #include <linux/delay.h>
 #include <linux/in.h>
 #include <linux/inet.h>
 #include <linux/spinlock.h>
 #include <rdma/ib_verbs.h>
 #include <rdma/rdma_cm.h>
 #include "server.h"
 #define SLAVE_REGISTER_SERVICE 1
 #define CLIENT_REQUEST_SERVICE 2
 #define MAX_MESSAGE_LENGTH 4096

 #define MASTER_CTX_INFO 100
 #define SLAVE_CTX_TO_MASTER_INFO 200
 #define SLAVE_CTX_TO_CLIENT_INFO 300
 #define CLIENT_CTX_INFO 400

 #define SLAVE_ALIVE   1
 #define SLAVE_REMOVED 2


 #define CLIENT_CTX_INFO_TO_MASTER  1
 #define CLIENT_CTX_INFO_TO_SLAVE   2
 #define CLIENT_SERACH_SLAVE_INFO   3

 #define SLAVE_CTX_INFO_TO_CLIENT   4
 #define MASTER_CTX_INFO_TO_CLIENT  5
 enum RDFS_CLIENT_REQUEST
 {
     RDFS_CLIENT_OPEN,
     RDFS_CLIENT_CLOSE,
     RDFS_CLIENT_MKDIR,
     RDFS_CLIENT_RMDIR,
     RDFS_CLIENT_LISTDIR
 };
 struct server_socket
 {
     struct socket * s_sock;// create
     struct socket * c_sock;// accept
 };
 struct service_info
 {
     char ip[IP_LEN];
     int port;
     struct ib_device* ib_dev;
     int service_type;
     struct server_socket  sock;
 };
 struct client_request_task
 {
    struct rdfs_message *message;
    struct socket * c_sock;
    struct client_request_task * next;
 };
 struct client_task
 {
     spinlock_t task_list_lock;
     struct client_request_task *task_list;
 };

 struct rdfs_message
 {
    int m_type;
    char m_data[MAX_MESSAGE_LENGTH];
 };

 int rdfs_recv_message(struct socket* sock,struct rdfs_context* ctx_p,int* m_type);
 int rdfs_send_message(struct socket* sock,struct rdfs_context* ctx_p,int m_type);
 int  send_data(struct socket * client_sock,char* data,int size);
 int recv_data(struct socket * client_sock,char* data,int size);
 #endif