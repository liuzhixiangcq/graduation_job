/* @author page
 * @date 2017/12/6
 * rdfs_master/rdfs_rdma.c
 */
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
 int rdfs_remove_context(struct rdfs_context * ctx_p,struct ib_device *dev)
 {
     /*
    if(ctx_p->qp)
    {
        ib_destry_qp(ctx_p->qp);
    }
    
    if(ctx_p->recv_cq)
    {

    }
    if(ctx_p->send_cq)
    {

    }
    */
    /*
    if(ctx_p->mr)
    {

    }
    */
    if(ctx_p->pd)
    {
        ib_dealloc_pd(dev);
    }
    kfree(ctx_p);
    return 0;
 }
 struct rdfs_context* rdfs_init_context(struct ib_device *dev)
 {
     rdfs_trace();
     if(dev == NULL)return NULL;

     struct ib_device_attr dev_attr;
     ib_query_device(dev,&dev_attr);

     struct rdfs_context *ctx_p = (struct rdfs_context*)kmalloc(sizeof(struct rdfs_context),GFP_KERNEL);
     if(rdfs_check_pointer(ctx_p))return NULL;

     ctx_p->pd = ib_alloc_pd(dev);
     if(rdfs_check_pointer(ctx_p->pd))return NULL;

     ctx_p->mr = ib_get_dma_mr(ctx_p->pd,IB_ACCESS_REMOTE_READ|IB_ACCESS_REMOTE_WRITE|IB_ACCESS_LOCAL_WRITE);
     if(rdfs_check_pointer(ctx_p->mr))return NULL;

     ctx_p->send_cq = ib_create_cq(dev,comp_handler_send,cq_event_handler_send,NULL,10,0);
     if(rdfs_check_pointer(ctx_p->send_cq))return NULL;

     ctx_p->recv_cq = ib_create_cq(dev,comp_handler_recv,cq_event_handler_recv,NULL,10,0);
     if(rdfs_check_pointer(ctx_p->recv_cq))return NULL;

     ib_req_notify_cq(ctx_p->recv_cq,IB_CQ_NEXT_COMP);
     ib_req_notify_cq(ctx_p->send_cq,IB_CQ_NEXT_COMP);

     memset(&ctx_p->qp_attr,0,sizeof(struct ib_qp_init_attr));
     ctx_p->qp_attr.send_cq = ctx_p->send_cq;
     ctx_p->qp_attr.recv_cq = ctx_p->recv_cq;
     ctx_p->qp_attr.cap.max_send_wr = INFINIBAND_MAX_SR_WR_NUMS;
     ctx_p->qp_attr.cap.max_recv_wr = INFINIBAND_MAX_SR_WR_NUMS;
     ctx_p->qp_attr.cap.max_send_sge = INFINIBAND_MAX_SR_SGE_NUMS;
     ctx_p->qp_attr.cap.max_recv_sge = INFINIBAND_MAX_SR_SGE_NUMS;
     ctx_p->qp_attr.cap.max_inline_data = 0;
     ctx_p->qp_attr.qp_type =  IB_QPT_RC;
     ctx_p->qp_attr.sq_sig_type = IB_SIGNAL_ALL_WR;

     ctx_p->qp = ib_create_qp(ctx_p->pd,&ctx_p->qp_attr);
     if(rdfs_check_pointer(ctx_p->qp))return NULL;

     struct ib_port_attr port_attr;
     ib_query_port(dev,INFINIBAND_DEV_PORT,&port_attr);
     ctx_p->rkey = ctx_p->mr->rkey;
     ctx_p->lid = port_attr.lid;
     ctx_p->qpn = ctx_p->qp->qp_num;
     get_random_bytes(&ctx_p->psn,sizeof(ctx_p->psn));
     ctx_p->psn &= 0xffffff;
     ctx_p->active_mtu = port_attr.active_mtu;

     ib_query_gid(dev,INFINIBAND_DEV_PORT,0,&ctx_p->gid);

     ctx_p->req_id.current_req_id = 0;
     ctx_p->req_id.lock_word = 0;
     spin_lock_init(&ctx_p->req_id.req_id_lock);
     return ctx_p;
 }