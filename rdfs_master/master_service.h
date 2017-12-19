/* @author page
 * @date 2017/12/19
 */

 #ifndef _RDFS_MASTER_SERVICE_H
 #define _RDFS_MASTER_SERVICE_H
 #include <linux/kernel.h>
 #include <linux/init.h>
 #include <linux/kthread.h>
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
 #include <linux/time.h>
 #include "server.h"
 #include "rdfs.h"
 #include "debug.h"
 #include "inode.h"
 #include "balloc.h"
 #include "network.h"
 #include "rdfs_config.h"
 #include "rdfs_rdma.h"

 int rdfs_init_slave_register_service(const char *ip,int port,struct ib_device * dev);
 int rdfs_init_client_request_service(const char *ip,int port,struct ib_device * dev);
 int rdfs_exit_service(struct task_struct* task);
 #endif