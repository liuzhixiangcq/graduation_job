/* @author page
 * @date 2017/12/6
 * rdfs_master/network.c
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
 #include "network.h"
 #include "server.h"
 static int recv_data(struct socket * client_sock,char* data,int size)
 {
     struct msghdr msg;
     struct kvec vec;
     int retval ;
     memset(&vec,0,sizeof(vec));
     memset(&msg,0,sizeof(msg));
     if(data == NULL)
     {
         printk("%s --> data is NULL\n",__FUNCTION__);
         return -1;
     }
     vec.iov_base = data;
     vec.iov_len = size;
     
     retval = kernel_recvmsg(client_sock,&msg,&vec,1,size,0);
     return retval;
 }
 static int  send_data(struct socket * client_sock,char* data,int size)
 {
     struct msghdr msg;
     struct kvec vec;
     int retval ;
     memset(&vec,0,sizeof(vec));
     memset(&msg,0,sizeof(msg));
     if(data == NULL)
     {
         printk("%s --> data is NULL\n",__FUNCTION__);
         return -1;
     }
     vec.iov_base = data;
     vec.iov_len = size;
     
     retval = kernel_sendmsg(client_sock,&msg,&vec,1,size);
     return retval;
     
 }
 
 int rdfs_sock_send_info(struct socket * sock,struct rdfs_context* ctx_p)
 {
     //char data[MAX_SOCK_BUFFER];
     struct rdfs_message message;
     int size;
     memset(message.m_data,0,MAX_MESSAGE_LENGTH);
     message.m_type = MASTER_CTX_INFO;
     sprintf(message.m_data,"%016Lx:%u:%x:%x:%lx:%lx",ctx_p->dma_addr,ctx_p->rkey,ctx_p->qpn,ctx_p->psn,ctx_p->lid,ctx_p->rem_block_nums);
     size = send_data(sock,&message,sizeof(message)); 
     printk("%s --> send_data: %016Lx:%u:%x:%x:%x:%x",__FUNCTION__,ctx_p->dma_addr,ctx_p->rkey,ctx_p->qpn,ctx_p->psn,ctx_p->lid,ctx_p->rem_block_nums);
     return size;
 }
 int rdfs_sock_get_info(struct socket * sock,struct rdfs_context* ctx_p)
 {
    // char data[MAX_SOCK_BUFFER];
     struct rdfs_message message;
     int size;
     memset(message.m_data,0,MAX_MESSAGE_LENGTH);
     size = recv_data(sock,&message,sizeof(message)); 
     sscanf(message.m_data,"%016Lx:%u:%x:%x:%x",&ctx_p->rem_addr,&ctx_p->rem_rkey,&ctx_p->rem_qpn,&ctx_p->rem_psn,&ctx_p->rem_lid);
     printk("%s --> receive_data: %016Lx:%u:%x:%x:%x",__FUNCTION__,ctx_p->rem_addr,ctx_p->rem_rkey,ctx_p->rem_qpn,ctx_p->rem_psn,ctx_p->rem_lid);
     return size;
 }
 int rdfs_send_message(struct socket* sock,struct rdfs_context* ctx_p,int m_type)
 {
    struct rdfs_message message;
    int size = 0;
    memset(message.m_data,0,MAX_MESSAGE_LENGTH);
    message.m_type = m_type;
    switch(m_type)
    {
        case MASTER_CTX_INFO:
            sprintf(message.m_data,"%016Lx:%u:%x:%x:%x",ctx_p->dma_addr,ctx_p->rkey,
            ctx_p->qpn,ctx_p->psn,ctx_p->lid);
            break;
        case SLAVE_CTX_TO_MASTER_INFO:
             sprintf(message.m_data,"%016Lx:%u:%x:%x:%lx:%lx",ctx_p->dma_addr,ctx_p->rkey,ctx_p->qpn,
             ctx_p->psn,ctx_p->lid,ctx_p->rem_block_nums);
        case SLAVE_CTX_TO_CLIENT_INFO:
             sprintf(message.m_data,"%016Lx:%u:%x:%x:%x",ctx_p->dma_addr,ctx_p->rkey,
             ctx_p->qpn,ctx_p->psn,ctx_p->lid);
             break;
        default:
            break;
    }
    size = send_data(sock,&message,sizeof(message));
    return size;
 }
 int rdfs_recv_message(struct socket* sock,struct rdfs_context* ctx_p,int* m_type)
 {
    struct rdfs_message message;
    int size = 0;
    memset(message.m_data,0,MAX_MESSAGE_LENGTH);
    size = recv_data(sock,&message,sizeof(message));
    *m_type = message.m_type;
    switch(message.m_type)
    {
        case MASTER_CTX_INFO:
             sscanf(message.m_data,"%016Lx:%u:%x:%x:%x",&ctx_p->rem_addr,&ctx_p->rem_rkey,&ctx_p->rem_qpn,
             &ctx_p->rem_psn,&ctx_p->rem_lid);
             break;
        case SLAVE_CTX_TO_MASTER_INFO:
             sscanf(message.m_data,"%016Lx:%u:%x:%x:%x:%x",&ctx_p->rem_addr,&ctx_p->rem_rkey,
             &ctx_p->rem_qpn,&ctx_p->rem_psn,&ctx_p->rem_lid,&ctx_p->rem_block_nums);
             break;
        case CLIENT_CTX_INFO:
             sscanf(message.m_data,"%016Lx:%u:%x:%x:%x:%x",&ctx_p->rem_addr,&ctx_p->rem_rkey,
             &ctx_p->rem_qpn,&ctx_p->rem_psn,&ctx_p->rem_lid);
             break;
        default:
             break;
    }
    return size;
 }