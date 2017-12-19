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
#include "event.h"
#include "rdfs.h"
#include "rdfs_sock.h"


int rdfs_client_handshake(struct socket *sock,struct rdfs_context* ctx_p);
int rdfs_server_handshake(struct socket* sock,struct rdfs_context* ctx_p);

int rdfs_modify_qp(struct rdfs_context* ctx_p)
{
    printk("%s\n",__FUNCTION__);
    int retval;
    struct ib_qp_attr attr;
    memset(&attr,0,sizeof(attr));

    attr.qp_state = IB_QPS_INIT;
    attr.pkey_index = 0;
    attr.port_num = 1;
    attr.qp_access_flags = IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ | IB_ACCESS_REMOTE_ATOMIC;
   
    printk("%s --> QP INIT\n",__FUNCTION__);
    retval = ib_modify_qp(ctx_p->qp,&attr,IB_QP_STATE|IB_QP_PKEY_INDEX|IB_QP_PORT|IB_QP_ACCESS_FLAGS);

    memset(&attr,0,sizeof(attr));
    attr.qp_state = IB_QPS_RTR;
    attr.path_mtu = ctx_p->active_mtu;
    attr.dest_qp_num = ctx_p->rem_qpn;
    attr.rq_psn = ctx_p->rem_psn;
    attr.max_dest_rd_atomic =1 ;
    attr.min_rnr_timer = 12;
    attr.ah_attr.dlid = ctx_p->rem_lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = 1;
    
    printk("%s --> QP RTS\n",__FUNCTION__);
    retval = ib_modify_qp(ctx_p->qp,&attr,IB_QP_STATE|IB_QP_AV|IB_QP_PATH_MTU|IB_QP_DEST_QPN|IB_QP_RQ_PSN|IB_QP_MAX_DEST_RD_ATOMIC|IB_QP_MIN_RNR_TIMER);

    attr.qp_state = IB_QPS_RTS;
    attr.timeout = 14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7;
    attr.sq_psn = ctx_p->psn;
    attr.max_rd_atomic = 1;
    printk("%s --> QP RTR\n",__FUNCTION__);
    retval =ib_modify_qp(ctx_p->qp,&attr,IB_QP_STATE|IB_QP_TIMEOUT|IB_QP_RETRY_CNT|IB_QP_RNR_RETRY|IB_QP_SQ_PSN|IB_QP_MAX_QP_RD_ATOMIC);
    return 0;
}

int rdfs_init_rdma_request(struct rdma_request * req,RDMA_OP op,u64 local_addr,u64 remote_addr,int size)
{
    printk("%s\n",__FUNCTION__);
    if(req == NULL)
    {
        printk("%s --> req is NULL\n",__FUNCTION__);
        return -1;
    }
    req->rw = op;
    req->local_dma_addr = local_addr;
    req->remote_dma_addr = remote_addr;
    req->size = size;
    return 0;
}
/*
int rdfs_post_wr(struct rdma_request * req)
{
    printk("%s\n",__FUNCTION__);
    if(req == NULL)
    {
        printk("%s --> req is NULL\n",__FUNCTION__);
        return -1;
    }
    struct ib_send_wr wr;
    struct ib_sge sg;
    struct ib_send_wr * bad_wr;
    memset(&sg,0,sizeof(sg));
    sg.addr = (uintptr_t)req->local_dma_addr;
    sg.length = req->size;
    sg.lkey = ctx_p->mr->lkey;

    memset(&wr,0,sizeof(struct ib_send_wr));
    
   // wr.wr_id = (uintptr_t)&ctx;
    wr.wr_id = 110;
    wr.sg_list = &sg;
    wr.num_sge = 1;
    wr.opcode = req->rw;
    wr.wr.rdma.remote_addr = req->remote_dma_addr;
    wr.wr.rdma.rkey = ctx_p->rem_rkey;

    if(ib_post_send(ctx_p->qp,&wr,&bad_wr))
    {
        printk("%s --> ib_post_send failed\n",__FUNCTION__);
        return -1;
    }
    return 0;
}
*/
struct rdfs_context* rdfs_init_context(struct ib_device *dev,struct socket* client_sock)
{
    printk("%s\n",__FUNCTION__);
    struct rdfs_context* ctx_p = NULL;
    ctx_p = (struct rdfs_context*)kmalloc(sizeof(struct rdfs_context),GFP_KERNEL);
    if(ctx_p == NULL)
    {
        printk("%s --> kmalloc rdfs_context failed\n",__FUNCTION__);
        return NULL;
    }
    ctx_p->ib_dev = dev;

    ib_query_device(dev,&(ctx_p->ib_dev_attr));
    /*
    INIT_IB_EVENT_HANDLER(&ieh,dev,async_event_handler);
    ib_register_event_handler(&ieh);
    */
    ib_query_port(dev,SLAVE_INFINIBAND_CARD_PORT,&(ctx_p->ib_port_attr));

    ctx_p->pd = ib_alloc_pd(dev);

    if(ctx_p->pd == NULL)
    {
        printk("%s --> ib_alloc_pd failed\n",__FUNCTION__);
        kfree(ctx_p);
        return NULL;
    }

    ctx_p->mr = ib_get_dma_mr(ctx_p->pd,IB_ACCESS_PERMITS);
    if(ctx_p->mr == NULL)
    {
        printk("%s --> ib_get_dma_mr failed\n",__FUNCTION__);
       // ib_dealloc_pd(ctx_p->pd);
        kfree(ctx_p);
        return NULL;
    }
    int retval;
    retval = rdfs_map_mem(ctx_p);
    if(retval)
    {
        printk("%s --> rdfs_map_mem failed\n",__FUNCTION__);
        kfree(ctx_p);
        return NULL;
    }
    ctx_p->send_cq = ib_create_cq(dev,comp_handler_send,cq_event_handler_send,NULL,10,0);
    ctx_p->recv_cq = ib_create_cq(dev,comp_handler_recv,cq_event_handler_recv,NULL,10,0);
    if(ctx_p->send_cq == NULL || ctx_p->recv_cq == NULL)
    {
        printk("%s --> ib_create_cq failed\n",__FUNCTION__);
        kfree(ctx_p);
        return NULL;
    }
    ib_req_notify_cq(ctx_p->recv_cq,IB_CQ_NEXT_COMP);
    ib_req_notify_cq(ctx_p->send_cq,IB_CQ_NEXT_COMP);
    
    memset(&(ctx_p->qp_attr),0,sizeof(struct ib_qp_init_attr));
    ctx_p->qp_attr.send_cq = ctx_p->send_cq;
    ctx_p->qp_attr.recv_cq = ctx_p->recv_cq;
    ctx_p->qp_attr.cap.max_send_wr = 10;
    ctx_p->qp_attr.cap.max_recv_wr = 10;
    ctx_p->qp_attr.cap.max_send_sge = 1;
    ctx_p->qp_attr.cap.max_recv_sge = 1;
    ctx_p->qp_attr.cap.max_inline_data = 0;
    ctx_p->qp_attr.qp_type =  IB_QPT_RC;
    ctx_p->qp_attr.sq_sig_type = IB_SIGNAL_ALL_WR;

    ctx_p->qp = ib_create_qp(ctx_p->pd,&ctx_p->qp_attr);
    if(ctx_p->qp == NULL)
    {
        printk("%s --> ib_create_qp failed\n",__FUNCTION__);
        kfree(ctx_p);
        return NULL;
    }
    ctx_p->rkey = ctx_p->mr->rkey;
    ctx_p->lid = ctx_p->ib_port_attr.lid;
    ctx_p->qpn = ctx_p->qp->qp_num;
    get_random_bytes(&ctx_p->psn,sizeof(ctx_p->psn));
    ctx_p->psn &= 0xffffff;
    ctx_p->active_mtu = ctx_p->ib_port_attr.active_mtu;

    ib_query_gid(dev,1,0,&ctx_p->gid);

    retval = rdfs_client_handshake(client_sock,ctx_p);

    if(retval)
    {
        printk("%s --> rdfs_sock_exchange_data failed\n",__FUNCTION__);
        kfree(ctx_p);
        return NULL;
    }
    
    retval = rdfs_modify_qp(ctx_p);
    if(retval)
    {
        printk("%s --> rdfs_modify_qp failed\n",__FUNCTION__);
        kfree(ctx_p);
        return NULL;
    }

    ib_req_notify_cq(ctx_p->send_cq,0);
    ib_req_notify_cq(ctx_p->recv_cq,0);

    int comp;
    struct ib_wc wc;

    comp = ib_poll_cq(ctx_p->send_cq,1,&wc);
    printk("%s ib_poll_cq:comp = %d wc.status= %d\n",__FUNCTION__,comp,wc.status);
    comp = ib_poll_cq(ctx_p->recv_cq,1,&wc);
    printk("%s ib_poll_cq:comp = %d wc.status= %d\n",__FUNCTION__,comp,wc.status);

    return ctx_p;
}



int rdfs_wait_register(void * ip_info,struct ib_device* dev)
{
    struct server_socket s_sock;
    struct server_socket* server_sock_p = &s_sock;	

    const char *server_ip = SLAVE_IP;
    int server_port = SLAVE_REGISTER_PORT;

    int retval;
    struct sockaddr_in server_addr;
    set_current_state(TASK_UNINTERRUPTIBLE);
    if(kthread_should_stop())
    {
        printk("%s --> kethread should stop \n",__FUNCTION__);
        return -1;
    }
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = in_aton(server_ip);

    printk("%s --> server_ip:%s port:%d\n",__FUNCTION__,server_ip,server_port);

    server_sock_p->server_sock = (struct socket*)kmalloc(sizeof(struct socket),GFP_KERNEL);
    server_sock_p->accept_sock = (struct socket*)kmalloc(sizeof(struct socket),GFP_KERNEL);
    retval = sock_create_kern(AF_INET,SOCK_STREAM,IPPROTO_TCP,&server_sock_p->server_sock);
    if(retval)
    {
        printk("%s --> sock_create_kern failed \n",__FUNCTION__);
        kfree(server_sock_p->server_sock);
        kfree(server_sock_p->accept_sock);
        return -1;
    }
    retval = kernel_bind(server_sock_p->server_sock,(struct sockaddr*)&server_addr,sizeof(struct sockaddr_in));
   
    if(retval)
    {
        printk("%s kernel_bind failed\n",__FUNCTION__);
        return -1;
    }
    retval = kernel_listen(server_sock_p->server_sock,SERVER_SOCK_LISTEN_NUMS);
  
    if(retval)
    {
        printk("%s kernel_listen failed\n",__FUNCTION__);
        return -1;
    }
    
    while(1)  
    {
        printk("%s --> slave wait for connection\n",__FUNCTION__);
        retval = kernel_accept(server_sock_p->server_sock,&server_sock_p->accept_sock,0);

        if(retval)
        {
            printk("%s kernel_accept failed\n",__FUNCTION__);
            continue;
        }
        struct rdfs_context* ctx_p;   
	    ctx_p = rdfs_init_context(dev,server_sock_p->accept_sock);
        if(ctx_p == NULL)
        {
            printk("%s rdfs_init_context failed\n",__FUNCTION__);
            continue;
        }
        //*************************//
        retval = rdfs_server_handshake(server_sock_p->accept_sock,ctx_p);
        if(retval)
        {
            printk("%s rdfs_sock_exchange_data1  failed\n",__FUNCTION__);
            continue;
        }
		rdfs_modify_qp(ctx_p);
	
    }

    return 0;

}


