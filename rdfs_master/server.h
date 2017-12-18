/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */
 #ifndef _RDFS_SERVER_H
 #define _RDFS_SERVER_H

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
#include <linux/spinlock.h>
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>

#include "rdfs.h"
#include "memory.h"
#define IP_LEN 20
#define SERVER_SOCK_LISTEN_NUMS 10
#define MAX_SOCK_BUFFER  1000
#define MAX_CLIENT_NUMS  10
#define RDFS_READ 0
#define RDFS_WRITE 1
#define IB_ACCESS_PERMITS (IB_ACCESS_REMOTE_READ|IB_ACCESS_REMOTE_WRITE|IB_ACCESS_LOCAL_WRITE)
#define MAX_PAGE_NUM 0x800000 //32GB / 4KB


/*
struct rdma_host_info
{
    struct ib_device* ib_dev;//
    struct ib_device_attr ib_dev_attr;//
    struct ib_port_attr ib_port_attr;//
};
*/
struct rdfs_req_id
{
     u64 current_req_id;
     unsigned long lock_word;
     spinlock_t req_id_lock;
};

struct rdfs_context
{
    struct ib_device *ib_dev;//
    struct ib_cq * send_cq,*recv_cq;
    struct ib_pd * pd;//
    struct ib_qp * qp;
    struct ib_qp_init_attr qp_attr;
    struct ib_mr *mr;//
    struct rdfs_req_id req_id;
    int active_mtu;//
    int lid;//
    int qpn;//
    int psn;//
    int qp_psn;
    uint32_t rkey;//
    union ib_gid gid;//
    char * rdma_buffer;
    u64 dma_addr;

    unsigned long long int rem_addr;
    uint32_t rem_rkey;
    int rem_qpn;
    int rem_psn;
    int rem_lid;
    unsigned long rem_block_nums;

};
typedef enum {RDMA_READ,RDMA_WRITE} RDMA_OP;
struct rdma_request
{
    RDMA_OP rw;
    u64 local_dma_addr;
    u64 remote_dma_addr;
    u64 req_id;
    int size;
};
u64 rdfs_mapping_address(char *vir_addr,int size,int ctx_idx);
void async_event_handler(struct ib_event_handler *ieh,struct ib_event *ie);
void comp_handler_send(struct ib_cq * cq,void * cq_context);
void comp_handler_recv(struct ib_cq * cq,void * cq_context);
//void test_handler_send(struct ib_cq * cq,void * cq_context);
void cq_event_handler_send(struct ib_event* ib_e,void * cq_context);
void cq_event_handler_recv(struct ib_event* ib_e,void * cq_context);
int rdfs_sock_get_client_info(struct server_socket *server_sock_p,int ctx_idx);
int rdfs_post_wr(int ctx_idx,struct rdma_request *req);
int rdfs_init_register_service(const char* ip,int server_ip,struct ib_device* dev);
int rdfs_init_request_service(const char *ip,int server_port,struct ib_device * dev);
int rdfs_sock_send_server_info(struct server_socket *server_sock_p,int ctx_idx);
int rdfs_sock_exchange_data(struct server_socket *server_sock_p,int ctx_idx);
int rdfs_modify_qp(int ctx_idx);
int rdfs_init_rdma_request(struct rdma_request * req,RDMA_OP op,u64 local_addr,u64 remote_addr,int size,u64 req_id);
int rdfs_block_rw(u64 rdfs_block,int rw,phys_addr_t phys,u64 req_id,int client_idx);
int rdfs_init_context(struct ib_device *dev);
int rdfs_wait_request(void * ip_info);
int rdfs_wait_register(void * ip_info);
int rdfs_init(void);
int rdfs_exit(void);
u64 rdfs_new_client_block(int client_idx);
extern struct rdfs_context ctx[MAX_CLIENT_NUMS];

void rdfs_rminit(struct rdfs_remote_memory *rrm, int remote_page_num);
int rdfs_rmalloc(struct rdfs_remote_memory *rrm);
int rdfs_rmfree(struct rdfs_remote_memory *rrm, int page_num);
int rdfs_get_rm_free_page_num(struct rdfs_remote_memory *rrm);
unsigned long dmfs_block_rw(u64 req_id,int client_idx,unsigned long start_addr,int rw,unsigned long size,phys_addr_t phys);

#endif
