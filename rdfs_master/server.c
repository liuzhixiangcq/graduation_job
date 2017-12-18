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
                req_id = req_word & SLAVE_ID_MASK;
                clear_bit_unlock(req_id,slave_ctx[s_id]->)
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

int rdfs_modify_qp(int s_id)
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
    retval = ib_modify_qp(slave_ctx[s_id]->qp,&attr,IB_QP_STATE|IB_QP_PKEY_INDEX|IB_QP_PORT|IB_QP_ACCESS_FLAGS);

    memset(&attr,0,sizeof(attr));
    attr.qp_state = slave_ctx[s_id]->active_mtu;
    attr.dest_qp_num = slave_ctx[s_id]->rem_qpn;
    attr.rq_psn = slave_ctx[s_id]->rem_psn;
    attr.max_dest_rd_atomic =1 ;
    attr.min_rnr_timer = 12;
    attr.ah_attr.dlid = slave_ctx[s_id]->rem_lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = 1;
    
    //printk("%s ready to rtr\n",__FUNCTION__);
    retval = ib_modify_qp(slave_ctx[s_id]->qp,&attr,IB_QP_STATE|IB_QP_AV|IB_QP_PATH_MTU|IB_QP_DEST_QPN|IB_QP_RQ_PSN|IB_QP_MAX_DEST_RD_ATOMIC|IB_QP_MIN_RNR_TIMER);

    attr.qp_state = IB_QPS_RTS;
    attr.timeout = 14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7;
    attr.sq_psn = slave_ctx[s_id]->psn;
    attr.max_rd_atomic = 1;
    //printk("%s ready to rts\n",__FUNCTION__);
    retval =ib_modify_qp(slave_ctx[s_id]->qp,&attr,IB_QP_STATE|IB_QP_TIMEOUT|IB_QP_RETRY_CNT|IB_QP_RNR_RETRY|IB_QP_SQ_PSN|IB_QP_MAX_QP_RD_ATOMIC);

    return 0;
}
u64 rdfs_mapping_address(char * vir_addr,int size,int s_id)
{
    rdfs_trace();
    u64 dma_addr;
    dma_addr = ib_dma_map_single(slave_ctx[s_id]->ib_dev,vir_addr,size,DMA_BIDIRECTIONAL);
    return dma_addr;
}

unsigned long rdfs_dma_block_rw(unsigned long local_phy_addr,unsigned long s_id,unsigned long block_id,unsigned long block_offset,int rw_flag)
{
    rdfs_trace();
    struct ib_send_wr wr;
    struct ib_sge sg;
    struct ib_send_wr * bad_wr;

    unsigned long size = RDFS_BLOCK_SIZE;
    unsigned long remote_phy_addr = slave_ctx[s_id]->rem_addr + block_id * RDFS_BLOCK_SIZE + block_offset;
    
    memset(&sg,0,sizeof(sg));
    sg.addr = (uintptr_t)local_phy_addr;
    sg.length = req->size;
    sg.lkey = slave_ctx[s_id]->mr->lkey;


    spin_lock(&slave_ctx[s_id]->req_id.req_id_lock);
    u64 req_id = slave_ctx[s_id]->req_id.current_req_id;
    slave_ctx[s_id]->req_id.current_req_id = (slave_ctx[s_id]->req_id.current_req_id+1)%64;
    spin_unlock(&slave_ctx[s_id]->req_id.req_id_lock);
    set_bit(req_id,&slave_ctx[s_id]->req_id.lock_word);

    memset(&wr,0,sizeof(struct ib_send_wr));

    wr.wr_id = (s_id << SLAVE_ID_SHIFT) & req_id;
    wr.sg_list = &sg;
    wr.num_sge = 1;
    wr.opcode = rw_flag;
    wr.send_flags = IB_SEND_SIGNALED;
    wr.wr.rdma.remote_addr = remote_phy_addr;
    wr.wr.rdma.rkey = slave_ctx[s_id]->rem_rkey;

    
    if(ib_post_send(slave_ctx[s_id]->qp,&wr,&bad_wr))
    {
        return -1;
    }
    wait_on_bit_lock(&slave_ctx[s_id]->req_id.lock_word,req_id,TASK_KILLABLE);
    return 0;
}




