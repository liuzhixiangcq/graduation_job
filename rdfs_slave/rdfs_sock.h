/* @author page
 * @date 2017/12/1
 */

 #ifndef _SOCK_H
 #define _SOCK_H

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
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>

#include "init.h"
#include "ctx.h"


int rdfs_sock_get_info(struct socket * sock,struct rdfs_context* ctx_p);
int rdfs_sock_send_info(struct socket * sock,struct rdfs_context* ctx_p);
int rdfs_client_handshake(struct socket *sock,struct rdfs_context* ctx_p);
int rdfs_server_handshake(struct socket* sock,struct rdfs_context* ctx_p);

#endif
