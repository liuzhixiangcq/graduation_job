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
#include "ctx.h"
#include "rdfs_sock.h"

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
    struct rdfs_message message;
    int size;
    memset(message.m_data,0,MAX_MESSAGE_LENGTH);
    message.m_type =  SLAVE_CTX_TO_MASTER_INFO;
    sprintf(message.m_data,"%016Lx:%u:%x:%x:%x:%lx",ctx_p->dma_addr,ctx_p->rkey,ctx_p->qpn,ctx_p->psn,ctx_p->lid,ctx_p->block_nums);
    size = send_data(sock,&message,sizeof(message)); 
    printk("%s --> send_data: dma_addr:%016Lx,rkey:%u,qpn:%x,psn:%x,lid:%x,block_nums:%lx\n",__FUNCTION__,ctx_p->dma_addr,ctx_p->rkey,ctx_p->qpn,ctx_p->psn,ctx_p->lid,ctx_p->block_nums);
    return size;
}


int rdfs_sock_get_info(struct socket * sock,struct rdfs_context* ctx_p)
{
    struct rdfs_message message;
    int size;
    memset(message.m_data,0,MAX_MESSAGE_LENGTH);
    size = recv_data(sock,&message,sizeof(message)); 
    sscanf(message.m_data,"%016Lx:%u:%x:%x:%x",&ctx_p->rem_addr,&ctx_p->rem_rkey,&ctx_p->rem_qpn,&ctx_p->rem_psn,&ctx_p->rem_lid);
    printk("%s --> receive_data: rem_addr:%016Lx,rkey:%u,qpn:%x,psn:%x,lid:%x",__FUNCTION__,ctx_p->rem_addr,ctx_p->rem_rkey,ctx_p->rem_qpn,ctx_p->rem_psn,ctx_p->rem_lid);
    return size;
}

int rdfs_client_handshake(struct socket *sock,struct rdfs_context* ctx_p)
{
    int retval;
    retval = rdfs_sock_send_info(sock,ctx_p);
    if(retval <= 0)
    {
        printk("%s rdfs_sock_send_client_info failed\n",__FUNCTION__);
        return -1;
    }
    retval = rdfs_sock_get_info(sock,ctx_p);
    if(retval <= 0)
    {
        printk("%s rdfs_sock_get_server_info failed\n",__FUNCTION__);
        return -1;
    }
    return 0;
}
int rdfs_server_handshake(struct socket* sock,struct rdfs_context* ctx_p)
{
    int retval;
    retval = rdfs_sock_get_info(sock,ctx_p);
    if(retval <= 0)
    {
        printk("%s rdfs_sock_get_client_info err\n",__FUNCTION__);
        return -1;
    }
    retval = rdfs_sock_send_info(sock,ctx_p);
    if(retval <= 0)
    {
        printk("%s rdfs_sock_send_server_info err\n",__FUNCTION__);
        return -1;
    }
    return 0;
}