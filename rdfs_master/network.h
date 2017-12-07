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

 #define SLAVE_REGISTER_SERVICE 1
 #define CLIENT_REQUEST_SERVICE 2
 #define MAX_MESSAGE_LENGTH 4096

 #define MASTER_CTX_INFO 100
 #define SLAVE_CTX_TO_MASTER_INFO 200
 #define SLAVE_CTX_TO_CLIENT_INFO 300
 #define CLIENT_CTX_INFO 400

 #define SLAVE_ALIVE   1
 #define SLAVE_REMOVED 2

 struct service_info
 {
     char ip[IP_LEN];
     int port;
     struct ib_device* ib_dev;
     int service_type;
     struct server_socket  sock;
 };
 struct server_socket
 {
     struct socket * s_sock;// create
     struct socket * c_sock;// accept
 };
 

 struct rdfs_message
 {
    int m_type;
    char m_data[MAX_MESSAGE_LENGTH];
 };
 #endif