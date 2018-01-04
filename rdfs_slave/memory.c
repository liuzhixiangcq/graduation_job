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
#include "init.h"
#include "memory.h"

u64 rdfs_mapping_address(char * vir_addr,u64 size,struct rdfs_context* ctx_p)
{
    u64 dma_addr;
    printk("%s\n",__FUNCTION__);
    dma_addr = ib_dma_map_single(ctx_p->ib_dev,vir_addr,size,DMA_BIDIRECTIONAL);
    if(dma_addr < SLAVE_MAPPING_PHY_ADDR)
    {
        printk("%s --> ib_dma_map_single failed dma_addr:%lx\n",__FUNCTION__,dma_addr);
        return 0;
    }
    return dma_addr;
}
void * rdfs_ioremap(u64 phy_addr,u64 size)
{
    void * retval = NULL;
    printk("%s\n",__FUNCTION__);
    retval = (void*)request_mem_region_exclusive(phy_addr,size,"DISAG_MEM");
    if(!retval)
    {
        printk("%s --> request_mem_region_exclusive failed\n",__FUNCTION__);
        return NULL;
    }
    retval = ioremap_cache(phy_addr,size);
    if(!retval)
    {
        printk("%s ioremap_cache failed\n",__FUNCTION__);
        return NULL;
    }
    return retval;
}
int map_flag = 0;
int map_block_nums = 0;
unsigned long map_dma_addr = 0;
int rdfs_map_mem(struct rdfs_context* ctx_p)
{
    if(map_flag)
    {
        ctx_p->block_nums = map_block_nums;
        ctx_p->dma_addr = map_dma_addr;
        return 0;
    }
    u64 phy_addr = SLAVE_MAPPING_PHY_ADDR;
    u64 size = SLAVE_MAPPING_PHY_SIZE;
    char* virt_addr;
    u64 dma_addr;

    ctx_p->block_nums = size/4096;
  
    virt_addr = (char*)rdfs_ioremap(phy_addr,size);
    if(virt_addr == NULL)
    {
        printk("%s --> rdfs_ioremap failed",__FUNCTION__);
        return -1;
    }

    virt_addr =__va(phy_addr);

    dma_addr = rdfs_mapping_address(virt_addr,size,ctx_p);
    if(dma_addr == 0)
    {
        printk("%s --> rdfs_mapping_address failed\n",__FUNCTION__);
        return -1;
    }
    ctx_p->dma_addr = dma_addr;
    map_block_nums = ctx_p->block_nums;
    map_dma_addr = ctx_p->dma_addr;
    //ctx_s.dma_addr = dma_addr;
	//ctx.dma_addr = dma_addr;
    map_flag = 1;
    return 0;
}
