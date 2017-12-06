/* @author page
 * @date 2017/12/5
 * rdfs_master/device.h
 */

 #ifndef _RDFS_DEVICE_H
 #define _RDFS_DEVICE_H

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
 #include "rdfs_config.h"

 int rdfs_init(void);
 int rdfs_exit(void);
 
 #endif