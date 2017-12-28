/* @author page
 * @date 2017/12/25
 */
#include "rdfs_config.h"
#include "network.h"
#include <infiniband/verbs.h>
extern  struct slave_context * slave_ctx[MAX_SLAVE_NUMS];
struct slave_context * rdfs_init_context()
{
    struct ibv_device **dev_list;
    struct ibv_device *ibv_dev;
    struct slave_context * ctx;
    int num_devices;
    dev_list = ibv_get_device_list(&num_devices);		
    ibv_dev = dev_list[RDFS_DEV_NO];
    
    ctx = (struct slave_context*)malloc(sizeof(struct slave_context));

    ctx->context = ibv_open_device(ibv_dev);					
    ctx->pd = ibv_alloc_pd(ctx->context);				
    ctx->event_channel = ibv_create_comp_channel(ctx->context);		
    ctx->send_cq = ibv_create_cq(ctx->context, 1000,ctx->event_channel,0,0); 	
    ctx->recv_cq = ibv_create_cq(ctx->context, 1000,ctx->event_channel,0,0); 

    struct ibv_qp_init_attr qp_attr;
    memset(&qp_attr, 0, sizeof(struct ibv_qp_init_attr));
    qp_attr.send_cq = ctx->send_cq;
    qp_attr.recv_cq = ctx->recv_cq;
    qp_attr.cap.max_send_wr  = 1000;
    qp_attr.cap.max_recv_wr  = 1000;
    qp_attr.cap.max_send_sge = 1;
    qp_attr.cap.max_recv_sge = 1;
    qp_attr.cap.max_inline_data = 0;
    qp_attr.qp_type = IBV_QPT_RC;

    ctx->qp = ibv_create_qp(ctx->pd, &qp_attr);
   	
    return ctx;		
}
int rdfs_modify_qp(struct slave_context* ctx)
{
    struct ibv_qp_attr attr;
    memset(&attr, 0, sizeof(attr));
    
    attr.qp_state        = IBV_QPS_INIT;
    attr.pkey_index      = 0;
    attr.port_num        = 1;
    attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE  | IBV_ACCESS_REMOTE_READ|IBV_ACCESS_REMOTE_ATOMIC ;
    int flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
    ibv_modify_qp(ctx->qp, &attr, flags);

    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTR;
    //attr.path_mtu = ctx->active_mtu;
    attr.dest_qp_num = ctx->rem_qpn;
    attr.rq_psn	    = ctx->rem_psn;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 12;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = ctx->rem_lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = 1;
    ibv_modify_qp(ctx->qp, &attr,IBV_QP_STATE|IBV_QP_AV|IBV_QP_PATH_MTU|IBV_QP_DEST_QPN|IBV_QP_RQ_PSN|IBV_QP_MAX_DEST_RD_ATOMIC |IBV_QP_MIN_RNR_TIMER);
   
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.sq_psn	 = ctx->psn;
    attr.timeout	 = 14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7; 
    attr.max_rd_atomic  = 1;
    ibv_modify_qp(ctx->qp, &attr,IBV_QP_STATE|IBV_QP_TIMEOUT|IBV_QP_RETRY_CNT|IBV_QP_RNR_RETRY|IBV_QP_SQ_PSN|IBV_QP_MAX_QP_RD_ATOMIC);    
    return 0;
}
int rdfs_init_slave_connect(unsigned int s_id,unsigned int ip,unsigned int port)
{
    int s_fd = rdfs_connect(ip,port);
    printf("connect to slave fd:%d\n",s_fd);
    slave_ctx[s_id] = rdfs_init_context();
    int m_type;
    rdfs_send_message(s_fd,slave_ctx[s_id],CLIENT_CTX_INFO_TO_SLAVE);
    rdfs_recv_message(s_fd,slave_ctx[s_id],&m_type);
    rdfs_modify_qp(slave_ctx[s_id]);
    return 0;
}
int rdfs_connect(unsigned int ip,int port)
{
    struct sockaddr_in sockaddr;
    int fd;
	if((fd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		printf("socket error!\n");
		return -1;
	}
	memset(&sockaddr,0,sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = ip;

	if((connect(fd,(struct sockaddr*)&sockaddr,sizeof(sockaddr))) == -1)
	{
		printf("connect error %s errno: %d\n",strerror(errno),errno);
		return -1;
	}
	return fd;
}
int rdfs_send_message(int sock_fd,struct slave_context* ctx_p,int m_type)
{
    struct rdfs_message message;
    int size = 0;
    memset(message.m_data,0,MAX_MESSAGE_LENGTH);
    message.m_type = m_type;
    switch(m_type)
    {
        case CLIENT_CTX_INFO_TO_SLAVE:
        sscanf(message.m_data,"%016Lx:%u:%x:%x:%lx",&ctx_p->dma_addr,&ctx_p->rkey,&ctx_p->qpn,
        &ctx_p->psn,&ctx_p->lid);
        printf("rem_addr:%lx rem_rkey:%lx rem_qpn:%lx rem_psn:%lx,rem_lid:%lx\n",ctx_p->dma_addr,
        ctx_p->rkey,ctx_p->qpn,ctx_p->lid);
            break;
        case CLIENT_CTX_INFO_TO_MASTER:
            break;
        case CLIENT_SERACH_SLAVE_INFO:
            break;
        default:
            break;
    }
    size = send_data(sock_fd,&message,sizeof(message));
    return size;
}
int rdfs_recv_message(int sock_fd,struct slave_context* ctx_p,int *m_type)
{
    struct rdfs_message message;
    int size = 0;
    memset(message.m_data,0,MAX_MESSAGE_LENGTH);
    size = recv_data(sock_fd,(void*)&message,sizeof(message));
    *m_type = message.m_type;
    unsigned int slave_num,slave_id,slave_ip,slave_port;
    switch(*m_type)
    {
        case SLAVE_CTX_TO_CLIENT:
        sscanf(message.m_data,"%016Lx:%u:%x:%x:%x:%lx",&ctx_p->rem_vaddr,&ctx_p->rem_rkey,
        &ctx_p->rem_qpn,&ctx_p->rem_psn,&ctx_p->rem_lid,&ctx_p->rem_block_nums);
        //printf("slave_ctx_to_client_info:rem_addr:%lx,rem_rkey:%u,rem_qpn:%x,rem_psn:%x,rem_lid:%x,block_nums:%lx\n",,ctx_p->rem_vaddr,
       // ctx_p->rem_rkey,ctx_p->rem_qpn,ctx_p->rem_psn,ctx_p->rem_lid,ctx_p->rem_block_nums);
            break;
        case CLIENT_SERACH_SLAVE_INFO:
            sscanf(message.m_data,"%u:%u:%u:%u",&slave_num,&slave_id,&slave_ip,&slave_port);
            printf("slave info:%d %d %d %d\n",slave_num,slave_id,slave_ip,slave_port);
            rdfs_init_slave_connect(slave_id,slave_ip,slave_port);
            if(slave_num>0)
                rdfs_recv_message(sock_fd,ctx_p,m_type);
            break;
        default:
            break;
    }
    return size;
}
int send_data(int fd,void* data,int size)
{
    return send(fd,data,size,0);
}
int recv_data(int fd,void* data,int size)
{
    return recv(fd,data,size,0);
}
