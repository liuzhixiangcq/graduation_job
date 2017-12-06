/* @author page
 * @date 2017/12/1
 */

 #ifndef _MEMORY_H
 #define _MEMORY_H

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

u64 rdfs_mapping_address(char * vir_addr,u64 size,struct rdfs_context* ctx_p);
int rdfs_map_mem(struct rdfs_context* ctx_p);
void * rdfs_ioremap(u64 phy_addr,u64 size);

#endif