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


#define OPEN_MODE 0 
#define READ_MODE 1
#define WRITE_MODE 2
#define CLOSE_MODE 4
#define OVER_MODE 3

extern struct rdfs_context * slave_ctx[MAX_SLAVE_NUMS];

void async_event_handler(struct ib_event_handler* ieh,struct ib_event* ie)
{
    rdfs_trace();
}
void comp_handler_send(struct ib_cq* cq,void *cq_context)
{
    rdfs_trace();
    struct ib_wc wc;
    u64 req_word = 0, s_id = 0,req_id = 0;
    do 
    {
        printk("%s ready to poll cq\n",__FUNCTION__);
        while(ib_poll_cq(cq,1,&wc)>0)
        {
            if(wc.status == IB_WC_SUCCESS)
            {
                req_word = wc.wr_id;
                printk("%s ib rdma success req_world:%lx\n",__FUNCTION__,req_word);
                s_id = req_word >> SLAVE_ID_SHIFT;
                req_id = req_word & REQ_ID_MASK;
                printk("s_id:%lx req_id:%lx lock_world:%lx\n",s_id,req_id,(slave_ctx[s_id]->req_id).lock_word);
                clear_bit_unlock(req_id,slave_ctx[s_id]->req_id.lock_word);
            }
            else
            {
               printk("%s FAILURE %d\n",__FUNCTION__,wc.status);
            }
        }
    }while(ib_req_notify_cq(cq,IB_CQ_NEXT_COMP|IB_CQ_REPORT_MISSED_EVENTS)>0);
}

void comp_handler_recv(struct ib_cq *cq,void * cq_context)
{
    rdfs_trace();
}
void cq_event_handler_send(struct ib_event* ib_e,void * cq_context)
{
    rdfs_trace();
}
void cq_event_handler_recv(struct ib_event* ib_e,void * cq_context)
{
    rdfs_trace();
}


int rdfs_modify_qp(int s_id)
{
    rdfs_trace();
    printk("%s modify ctx sid:%d\n",__FUNCTION__,s_id);
    int retval;
    struct ib_qp_attr attr;
    memset(&attr,0,sizeof(attr));

    attr.qp_state = IB_QPS_INIT;
    attr.pkey_index = 0;
    attr.port_num = 1;
    attr.qp_access_flags = IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ| IB_ACCESS_REMOTE_ATOMIC;
    printk("%s ready to init\n",__FUNCTION__);
    retval = ib_modify_qp(slave_ctx[s_id]->qp,&attr,IB_QP_STATE|IB_QP_PKEY_INDEX|IB_QP_PORT|IB_QP_ACCESS_FLAGS);
    printk("%s qp init retval:%d\n",__FUNCTION__,retval);

    memset(&attr,0,sizeof(attr));
    attr.qp_state = IB_QPS_RTR;
    attr.path_mtu = slave_ctx[s_id]->active_mtu;
    attr.dest_qp_num = slave_ctx[s_id]->rem_qpn;
    attr.rq_psn = slave_ctx[s_id]->rem_psn;
    attr.max_dest_rd_atomic =1 ;
    attr.min_rnr_timer = 12;
    attr.ah_attr.dlid = slave_ctx[s_id]->rem_lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = 1;
    
    printk("%s ready to rtr\n",__FUNCTION__);
    retval = ib_modify_qp(slave_ctx[s_id]->qp,&attr,IB_QP_STATE|IB_QP_AV|IB_QP_PATH_MTU|IB_QP_DEST_QPN|IB_QP_RQ_PSN|IB_QP_MAX_DEST_RD_ATOMIC|IB_QP_MIN_RNR_TIMER);
    printk("%s qp rtr retval:%d\n",__FUNCTION__,retval);

    attr.qp_state = IB_QPS_RTS;
    attr.timeout = 14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7;
    attr.sq_psn = slave_ctx[s_id]->psn;
    attr.max_rd_atomic = 1;
    printk("%s ready to rts\n",__FUNCTION__);
    retval =ib_modify_qp(slave_ctx[s_id]->qp,&attr,IB_QP_STATE|IB_QP_TIMEOUT|IB_QP_RETRY_CNT|IB_QP_RNR_RETRY|IB_QP_SQ_PSN|IB_QP_MAX_QP_RD_ATOMIC);
    printk("%s qp rts retval:%d\n",__FUNCTION__,retval);
    return 0;
}
u64 rdfs_mapping_address(char * vir_addr,int size)
{
    rdfs_trace();
    u64 dma_addr;
    extern struct ib_device * global_ib_dev;
    dma_addr = ib_dma_map_single(global_ib_dev,vir_addr,size,DMA_BIDIRECTIONAL);
    return dma_addr;
}

unsigned long rdfs_rdma_block_rw(unsigned long local_phy_addr,unsigned long s_id,unsigned long block_id,unsigned long block_offset,unsigned long size,int rw_flag)
{
    rdfs_trace();
    struct ib_send_wr wr;
    struct ib_sge sg;
    struct ib_send_wr * bad_wr;
    printk("%s 1\n",__FUNCTION__);
    unsigned long remote_phy_addr = slave_ctx[s_id]->rem_addr + block_id * RDFS_BLOCK_SIZE + block_offset;
    
    memset(&sg,0,sizeof(sg));
    sg.addr = (uintptr_t)local_phy_addr;
    sg.length = size;
    sg.lkey = slave_ctx[s_id]->mr->lkey;

    printk("%s 2\n",__FUNCTION__);
    spin_lock(&(slave_ctx[s_id]->req_id.req_id_lock));
    u64 req_id = slave_ctx[s_id]->req_id.current_req_id;
    slave_ctx[s_id]->req_id.current_req_id = (slave_ctx[s_id]->req_id.current_req_id+1)%64;
    spin_unlock(&(slave_ctx[s_id]->req_id.req_id_lock));
    set_bit(req_id,&slave_ctx[s_id]->req_id.lock_word);

    memset(&wr,0,sizeof(struct ib_send_wr));
    printk("%s 3\n",__FUNCTION__);
    wr.wr_id = (s_id << SLAVE_ID_SHIFT) & req_id;
    wr.sg_list = &sg;
    wr.num_sge = 1;
    wr.opcode = rw_flag;
    wr.send_flags = IB_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = remote_phy_addr;
    wr.wr.rdma.rkey = slave_ctx[s_id]->rem_rkey;
    printk("%s 4\n",__FUNCTION__);
    printk("%s s_id:%lx req_id:%lx remote_addr:%lx local_addr:%lx lock_world:%lx\n",__FUNCTION__,s_id,req_id,remote_phy_addr,local_phy_addr,slave_ctx[s_id]->req_id.lock_word);
    if(ib_post_send(slave_ctx[s_id]->qp,&wr,&bad_wr))
    {
        return -1;
    }
    printk("wait..........\n");
    wait_on_bit_lock(&slave_ctx[s_id]->req_id.lock_word,req_id,TASK_KILLABLE);
    printk("wait end!!!!!!\n");
    return size ;
}




