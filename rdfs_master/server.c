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
struct rdma_host_info host_info;
struct rdfs_context ctx[MAX_CLIENT_NUMS];
struct ib_event_handler ieh;
int ctx_cnt = 0;
int is_modify = 0;
int posted_req_cnt = 0;
extern int current_client;
#define OPEN_MODE 0 
#define READ_MODE 1
#define WRITE_MODE 2
#define CLOSE_MODE 4
#define OVER_MODE 3

extern char __user *current_buf;
extern unsigned long current_length;
extern unsigned long dma_start_addr;
struct task_struct *tsk;

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
        while(ib_poll_cq(cq,1,&wc)>0)
        {
            if(wc.status == IB_WC_SUCCESS)
            {
                req_word = wc.wr_id;
                s_id = req_word >> SLAVE_ID_SHIFT;
                clear_bit_unlock(0,slave_ctx[s_id]->)
		       // clear_bit_unlock(req_id,&ctx[current_client].max_req_id.lock_word);
		       // wake_up_bit(&ctx[current_client].max_req_id.lock_word,req_id);
		       // posted_req_cnt--;
		       /// if(current_length > 0)
		       // {	
			  //      ssize_t ret = __copy_to_user(current_buf,rdfs_va(dma_start_addr),current_length);
			 //       current_length = 0;
		      //  }
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

static void recv_data(struct server_socket* server_sock_p,char* data,int size)
{
    rdfs_trace();
    struct msghdr msg;
    struct kvec vec;
    memset(&vec,0,sizeof(vec));
    memset(&msg,0,sizeof(msg));
    if(data == NULL)
    {
        //printk("%s data is NULL\n",__FUNCTION__);
        return ;
    }
    vec.iov_base = data;
    vec.iov_len = size;
    int retval ;
    retval = kernel_recvmsg(server_sock_p->accept_sock,&msg,&vec,1,size,0);
    //printk("%s recv msg:%d Bytes\n",__FUNCTION__,retval);
}
static void send_data(struct server_socket* server_sock_p,char* data,int size)
{
    rdfs_trace();
    struct msghdr msg;
    struct kvec vec;
    memset(&vec,0,sizeof(vec));
    memset(&msg,0,sizeof(msg));
    if(data == NULL)
    {
        //printk("%s data is NULL\n",__FUNCTION__);
        return ;
    }
    vec.iov_base = data;
    vec.iov_len = size;
    int retval ;
    retval = kernel_sendmsg(server_sock_p->accept_sock,&msg,&vec,1,size);
    //printk("%s send msg:%d Bytes\n",__FUNCTION__,retval);
}
int rdfs_sock_get_client_info(struct server_socket *server_sock_p,int ctx_idx)
{
    rdfs_trace();
    char data[MAX_SOCK_BUFFER];
    memset(data,0,MAX_SOCK_BUFFER);
    recv_data(server_sock_p,data,MAX_SOCK_BUFFER); 
    sscanf(data,"%016Lx:%u:%x:%x:%x:%x",&ctx[ctx_idx].rem_addr,&ctx[ctx_idx].rem_rkey,&ctx[ctx_idx].rem_qpn,&ctx[ctx_idx].rem_psn,&ctx[ctx_idx].rem_lid,&ctx[ctx_idx].rem_block_nums);
    //printk("%s receive_data:\n %016Lx:%u:%x:%x:%x:%x",__FUNCTION__,ctx[ctx_idx].rem_addr,ctx[ctx_idx].rem_rkey,ctx[ctx_idx].rem_qpn,ctx[ctx_idx].rem_psn,ctx[ctx_idx].rem_lid,ctx[ctx_idx].rem_block_nums);
    return 0;
}
int rdfs_sock_send_server_info(struct server_socket *server_sock_p,int ctx_idx)
{
    rdfs_trace();
    char data[MAX_SOCK_BUFFER];
    memset(data,0,MAX_SOCK_BUFFER);
    sprintf(data,"%016Lx:%u:%x:%x:%x",ctx[ctx_idx].dma_addr,ctx[ctx_idx].rkey,ctx[ctx_idx].qpn,ctx[ctx_idx].psn,ctx[ctx_idx].lid);
    send_data(server_sock_p,data,MAX_SOCK_BUFFER); 
    //printk("%s send_data:\n %016Lx:%u:%x:%x:%x",__FUNCTION__,ctx[ctx_idx].dma_addr,ctx[ctx_idx].rkey,ctx[ctx_idx].qpn,ctx[ctx_idx].psn,ctx[ctx_idx].lid);
    return 0;
}
int rdfs_sock_exchange_data(struct server_socket *server_sock_p,int ctx_idx)
{
    rdfs_trace();
    int retval;
    retval = rdfs_sock_get_client_info(server_sock_p,ctx_idx);
    if(retval)
    {
        //printk("%s rdfs_sock_get_client_info err\n",__FUNCTION__);
        return -1;
    }
    retval = rdfs_sock_send_server_info(server_sock_p,ctx_idx);
    if(retval)
    {
        //printk("%s rdfs_sock_send_server_info err\n",__FUNCTION__);
        return -1;
    }
    return 0;
}

static void recv_data1(struct socket *accept_sock,char* data,int size)
{
    rdfs_trace();
    struct msghdr msg;
    struct kvec vec;
    memset(&vec,0,sizeof(vec));
    memset(&msg,0,sizeof(msg));
    if(data == NULL)
    {
        return ;
    }
    vec.iov_base = data;
    vec.iov_len = size;
    int retval ;
    retval = kernel_recvmsg(accept_sock,&msg,&vec,1,size,0);
}

static void send_data1(struct socket *accept_sock,char* data,int size)
{
    rdfs_trace();
    struct msghdr msg;
    struct kvec vec;
    memset(&vec,0,sizeof(vec));
    memset(&msg,0,sizeof(msg));
    if(data == NULL)
    {
        return ;
    }
    vec.iov_base = data;
    vec.iov_len = size;
    int retval ;
    retval = kernel_sendmsg(accept_sock,&msg,&vec,1,size);
}

int rdfs_sock_client_exchange_data(struct socket *accept_sock,int ctx_idx)
{
    rdfs_trace();

    struct timeval s1,s2,e1,e2,e3,e4;
    do_gettimeofday(&s1);

    printk("%s  wait to recv data   ctx_idx = %d \n",__FUNCTION__,ctx_idx);
    int retval;
    unsigned long ino = 0;
    unsigned long mode = 5;
    unsigned long offset = 0;
    unsigned long length = 0;
    char sdata[MAX_SOCK_BUFFER];
    char rdata[MAX_SOCK_BUFFER];
    char path[100];
    memset(rdata,0,MAX_SOCK_BUFFER);
    //printk("ready to recv data\n");
    printk("start recv ...\n");
    recv_data1(accept_sock,rdata,MAX_SOCK_BUFFER); 
    printk("ctx_idx = %d   rdata :%s\n",ctx_idx, rdata);

    sscanf(rdata,"%ld:%016Lx:%u:%x:%x:%x:%ld:%ld:%ld:%s",&mode,&ctx[ctx_idx].rem_addr,&ctx[ctx_idx].rem_rkey,&ctx[ctx_idx].rem_qpn,&ctx[ctx_idx].rem_psn,&ctx[ctx_idx].rem_lid,&ino,&offset,&length,path);

    do_gettimeofday(&s1);

    if(mode == OVER_MODE || mode == 5)
    {
	    return -1;
    }
    if(mode == CLOSE_MODE)
    {

	    struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	    struct inode* inode = rdfs_iget(sb, ino);
	    i_size_write(inode, length);
	    return 0;
    }

    if(mode == OPEN_MODE)
    {	
	    printk("mode = %ld  open\n",mode);
	    printk("path : %s\n",path);

	    char nodata[100] = "aaa" ;
	    ino = path_to_inode(path);

	    unsigned long i_size = 0;
	    if(ino != 0)
	    {
		    struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
		    struct inode* inode = rdfs_iget(sb, ino);
		    i_size = i_size_read(inode);
		    printk("i_size = %ld\n",i_size);
	    }

    	memset(sdata,0,MAX_SOCK_BUFFER);
	    sprintf(sdata,"%016Lx:%u:%x:%x:%x:%x:%ld:%s",ctx[ctx_idx].dma_addr,ctx[ctx_idx].rkey,ctx[ctx_idx].qpn,ctx[ctx_idx].psn,ctx[ctx_idx].lid,ino,i_size,nodata);
    	
    	send_data1(accept_sock,sdata,MAX_SOCK_BUFFER);
	

	    do_gettimeofday(&e1);
	
    	return 0;
    }
    else
    {
	    if(ino == 0)
        {
		    printk("ino = 0 !  please check\n");
		    return 0;
        }
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	struct inode *inode = rdfs_iget(sb,ino);
	struct rdfs_inode_info *ni_info = RDFS_I(inode);
	struct rdfs_inode* ni = get_rdfs_inode(sb,ino);

	do_gettimeofday(&s2);

	if(mode == WRITE_MODE)
	{
		long pages_exist = 0, pages_to_alloc = 0,pages_needed = 0; 
		loff_t size = ni_info->file_size;
		pages_needed = ((offset + length + sb->s_blocksize - 1) >> sb->s_blocksize_bits);
	    pages_exist = (size + sb->s_blocksize - 1) >> sb->s_blocksize_bits;
	    pages_to_alloc = pages_needed - pages_exist;

		if(pages_to_alloc > 0)
		{
			size_t retval = dmfs_new_block(ni_info,pages_to_alloc * FK_SIZE);
            if (retval < 0)
            {
				rdfs_info("alloc blocks failed!\n");
			}
			size = i_size_read(inode);
			printk("vfs inode->i_size : %ld\n",size);
		}
	}

	do_gettimeofday(&e2);

	struct index_search_result *result = kmalloc(sizeof(struct index_search_result) * MAX_BLOCK_NUM,GFP_KERNEL);

	unsigned long num = dmfs_index_search(ni_info,offset,length,result);

	do_gettimeofday(&e3);
	char sdata1[MAX_SOCK_BUFFER];
    memset(sdata1,0,MAX_SOCK_BUFFER);

	char index_data[MAX_SOCK_BUFFER];
	memset(index_data,0,MAX_SOCK_BUFFER);
    unsigned long len = 0;
    int i;
	for(i = 0;i<num;i++)
	{
		len = strlen(index_data);
		//printk("len = %ld\n",len);
		sprintf(index_data + len, "%ld:%ld:%ld:%ld:%ld:",result[i].slave_id, result[i].start_addr, result[i].size, result[i].block_type, result[i].file_offset);
	}

	sprintf(sdata1,"%016Lx:%u:%x:%x:%x:%x:%ld:%s",ctx[ctx_idx].dma_addr,ctx[ctx_idx].rkey,ctx[ctx_idx].qpn,ctx[ctx_idx].psn,ctx[ctx_idx].lid,ino,num,index_data);

    send_data1(accept_sock,sdata1,MAX_SOCK_BUFFER);
	do_gettimeofday(&e4);
	return 0;
    }
    return 0;
}

int rdfs_modify_qp(int ctx_idx)
{
    rdfs_trace();
    int retval;
    struct ib_qp_attr attr;
    memset(&attr,0,sizeof(attr));

    attr.qp_state = IB_QPS_INIT;
    attr.pkey_index = 0;
    attr.port_num = 1;
    attr.qp_access_flags = IB_ACCESS_REMOTE_WRITE | IB_ACCESS_REMOTE_READ| IB_ACCESS_REMOTE_ATOMIC;
    //printk("%s ready to init\n",__FUNCTION__);
    retval = ib_modify_qp(ctx[ctx_idx].qp,&attr,IB_QP_STATE|IB_QP_PKEY_INDEX|IB_QP_PORT|IB_QP_ACCESS_FLAGS);

    memset(&attr,0,sizeof(attr));
    attr.qp_state = IB_QPS_RTR;
    attr.path_mtu = ctx[ctx_idx].active_mtu;
    attr.dest_qp_num = ctx[ctx_idx].rem_qpn;
    attr.rq_psn = ctx[ctx_idx].rem_psn;
    attr.max_dest_rd_atomic =1 ;
    attr.min_rnr_timer = 12;
    attr.ah_attr.dlid = ctx[ctx_idx].rem_lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = 1;
    
    //printk("%s ready to rtr\n",__FUNCTION__);
    retval = ib_modify_qp(ctx[ctx_idx].qp,&attr,IB_QP_STATE|IB_QP_AV|IB_QP_PATH_MTU|IB_QP_DEST_QPN|IB_QP_RQ_PSN|IB_QP_MAX_DEST_RD_ATOMIC|IB_QP_MIN_RNR_TIMER);

    attr.qp_state = IB_QPS_RTS;
    attr.timeout = 14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7;
    attr.sq_psn = ctx[ctx_idx].psn;
    attr.max_rd_atomic = 1;
    //printk("%s ready to rts\n",__FUNCTION__);
    retval =ib_modify_qp(ctx[ctx_idx].qp,&attr,IB_QP_STATE|IB_QP_TIMEOUT|IB_QP_RETRY_CNT|IB_QP_RNR_RETRY|IB_QP_SQ_PSN|IB_QP_MAX_QP_RD_ATOMIC);

    return 0;
}
u64 rdfs_mapping_address(char * vir_addr,int size,int ctx_idx)
{
    rdfs_trace();
    u64 dma_addr;
    dma_addr = ib_dma_map_single(ctx[ctx_idx].ib_dev,vir_addr,size,DMA_BIDIRECTIONAL);
    return dma_addr;
}
int rdfs_init_rdma_request(struct rdma_request * req,RDMA_OP op,u64 local_addr,u64 remote_addr,int size,u64 req_id)
{
    rdfs_trace();
    if(req == NULL)
    {
        return -1;
    }
    req->rw = op;
    req->local_dma_addr = local_addr;
    req->remote_dma_addr = remote_addr;
    req->size = size;
    req->req_id = req_id;
    return 0;
}
int rdfs_post_wr(int ctx_idx,struct rdma_request * req)
{
    rdfs_trace();
    if(req == NULL)
    {
        return -1;
    }
    struct ib_send_wr wr;
    struct ib_sge sg;
    struct ib_send_wr * bad_wr;
    memset(&sg,0,sizeof(sg));
    sg.addr = (uintptr_t)req->local_dma_addr;
    sg.length = req->size;
    sg.lkey = ctx[ctx_idx].mr->lkey;

    memset(&wr,0,sizeof(struct ib_send_wr));
    wr.wr_id = req->req_id;
    wr.sg_list = &sg;
    wr.num_sge = 1;
    if(req->rw == RDMA_READ)
    wr.opcode = IB_WR_RDMA_READ;
    else
    wr.opcode = IB_WR_RDMA_WRITE;
    wr.send_flags = IB_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = req->remote_dma_addr;
    wr.wr.rdma.rkey = ctx[ctx_idx].rem_rkey;

    if(ib_post_send(ctx[ctx_idx].qp,&wr,&bad_wr))
    {
        return -1;
    }
    return 0;
}


struct thread_args
{
	int ctx_idx;
	struct socket *server_sock;
	struct socket *accept_sock;
};


void *thread_fun(void *args)
{
    rdfs_trace();
	int ctx_idx = 0;
	int retval;
	struct thread_args *my_args;
	my_args = (struct thread_args *)args;
	ctx_idx = my_args->ctx_idx;
	printk("ctx_idx = %d\n",ctx_idx);
	printk("my_args->accept_sock = %lx\n",my_args->accept_sock);
		
	while(1){
		retval = rdfs_sock_client_exchange_data(my_args->accept_sock,ctx_idx);
		if(retval < 0)
		{
			printk("ctx_idx = %d  retval = %d\n", ctx_idx, retval);
		    	break;
		}
	}
	rdfs_modify_qp(ctx_idx);
	printk("%s over  ctx_idx = %d\n",__FUNCTION__,ctx_idx);
	printk("---------------------cut line--------------------\n");
	return NULL;
}

static u64 rdfs_sn=0;
u64 rdfs_new_client_block(int client_idx)
{
	rdfs_trace();
	return rdfs_rmalloc(&ctx[client_idx].rrm);
}
unsigned long rdfs_rdma_block_rw(unsigned long local_dma_addr,unsigned long s_id,unsigned long block_id,unsigned long block_offset,int rw_flag)
{
    struct rdma_request req;
    u64 size = RDFS_BLOCK_SIZE;
    req.local_dma_addr = local_dma_addr;
    req.remote_dma_addr = slave_ctx[s_id]->rem_addr + block_id * size + block_offset;

}
int rdfs_block_rw(u64 rdfs_block,int rw,phys_addr_t phys,u64 req_id,int client_idx)
{
	struct rdma_request req;
	rdfs_trace();
	int retval;
	int ctx_idx=client_idx;
	u64 size = 4096;
	req.local_dma_addr = phys;
	req.remote_dma_addr = ctx[ctx_idx].rem_addr + rdfs_block*size;
	req.req_id = req_id;
	if(rw == RDFS_READ)
	req.rw = RDMA_READ;
	else 
	req.rw = RDMA_WRITE;
	req.size = size;
	//printk("%s req_id:%ld req_local_addr:%lx req_remote_addr:%lx\n",__FUNCTION__,req_id,phys,req.remote_dma_addr);
	rdfs_post_wr(ctx_idx,&req);
	return 0;
}
unsigned long dmfs_block_rw(u64 req_id,int client_idx,unsigned long start_addr,int rw,unsigned long size,phys_addr_t phys)
{
    rdfs_trace();
	struct rdma_request req;
	//printk("%s  req_id = %ld  client_idx = %d  start_addr = %lx  size = %ld  phys = %lx\n",__FUNCTION__,req_id,client_idx,start_addr,size,phys);
	int retval;
	int ctx_idx=client_idx;
	req.local_dma_addr = phys;
	req.remote_dma_addr = start_addr;
	req.req_id = req_id;
	if(rw == RDFS_READ)
		req.rw = RDMA_READ;
	else 
		req.rw = RDMA_WRITE;
	req.size = size;
	rdfs_post_wr(ctx_idx,&req);
	return size;
}


int rdfs_rw_test(void)
{
    rdfs_trace();
    struct rdma_request req;
    int retval;
    int ctx_idx=0;
    int size = 4096;
    char * read_buf = (char*)kmalloc(size,GFP_KERNEL);
    req.local_dma_addr = rdfs_mapping_address(read_buf,size,ctx_idx);
    req.remote_dma_addr = ctx[ctx_idx].rem_addr;
    req.rw = RDMA_READ;
    req.size = size;
    rdfs_post_wr(ctx_idx,&req);
    
}



void rdfs_rminit(struct rdfs_remote_memory *rrm, int remote_page_num)
{
    rdfs_trace();
    int i;
    rrm->page_num = remote_page_num;
    rrm->free_page_num = remote_page_num;
    rrm->freelist_head = 0;
    for(i = 1; i < remote_page_num; i++)
    {
        rrm->pages[i - 1] = i;
    }
    rrm->pages[remote_page_num - 1] = -1;
}

int rdfs_rmalloc(struct rdfs_remote_memory *rrm)
{   
    rdfs_trace();
    int return_value;
    if(rrm->free_page_num)
    {
        return_value = rrm->freelist_head;
        rrm->freelist_head = rrm->pages[return_value];
        rrm->free_page_num--;
        rrm->pages[return_value] = -2;
	//printk("alloc page %d...\n",return_value);
        return return_value;
    } else {
        //printk("no free page left!\n");
        return -1;
    }
}
int rdfs_rmfree(struct rdfs_remote_memory *rrm, int page_num)
{
    rdfs_trace();
    if(rrm->pages[page_num] != -2)
    {
        //printk("trying to free a free page!\n");
        return -1;
    }
    if(rrm->free_page_num)
    {
        rrm->pages[page_num] = rrm->freelist_head;
    } else {
        rrm->pages[page_num] = -1;
    }
    rrm->free_page_num++;
    rrm->freelist_head = page_num;
    return 0;
}

int rdfs_get_rm_free_page_num(struct rdfs_remote_memory *rrm)
{
    rdfs_trace();
    return rrm->free_page_num;
}

