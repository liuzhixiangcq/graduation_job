/* @author page
 * @date 2017/11/30
 */
 #ifndef _RDFS_H
 #define _RDFS_H

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

int rdfs_init_rdma_request(struct rdma_request * req,RDMA_OP op,u64 local_addr,u64 remote_addr,int size);
//int rdfs_post_wr(struct rdma_request * req);
int rdfs_modify_qp(struct rdfs_context* ctx_p);
struct rdfs_context* rdfs_init_context(struct ib_device *dev,struct socket* client_sock);
int rdfs_wait_register(void * ip_info,struct ib_device* dev);

#endif

