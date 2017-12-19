/* @author page
 * date 2017/12/19
 */

 #ifndef _RDFS_RDMA_H
 #define _RDFS_RDMA_H
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

 int rdfs_remove_context(struct rdfs_context * ctx_p,struct ib_device *dev);
 struct rdfs_context* rdfs_init_context(struct ib_device *dev);
 #endif