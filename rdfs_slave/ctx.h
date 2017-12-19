/* @author page
 * @date 2017/11/30
 */
 #ifndef _CTX_H
 #define _CTX_H

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
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>
#define IP_LEN 20
#define MAX_SOCK_BUFFER 1000
#define SERVER_SOCK_LISTEN_NUMS 10
#define MAX_CLIENT_NUMS 10
#define IB_ACCESS_PERMITS (IB_ACCESS_REMOTE_READ|IB_ACCESS_REMOTE_WRITE|IB_ACCESS_LOCAL_WRITE)
#define MAX_PAGE_NUM 0x800000 //32GB / 4KB
#define MAX_MESSAGE_LENGTH 4096
#define SLAVE_CTX_TO_MASTER_INFO 200
struct server_socket
{
    struct socket * server_sock;
    struct socket * accept_sock;
};
struct ip_info
{
    char ip[IP_LEN];
    int port;
    struct ib_device *ib_dev;
};

struct rdfs_context
{
    struct ib_device * ib_dev;
    struct ib_cq * send_cq,*recv_cq;
    struct ib_pd * pd;
    struct ib_qp * qp;
    struct ib_qp_init_attr qp_attr;
    struct ib_mr * mr;

    struct ib_device_attr ib_dev_attr;
    struct ib_port_attr ib_port_attr;
    
    int active_mtu;
    unsigned long block_nums;
    int lid;
    int qpn;
    int psn;
    int qp_psn;
    uint32_t rkey;
    union ib_gid gid;
    char * rdma_buffer;
    u64 dma_addr;

    unsigned long long int rem_addr;
    uint32_t rem_rkey;
    int rem_qpn;
    int rem_psn;
    int rem_lid;
};

typedef enum {RDMA_READ,RDMA_WRITE}RDMA_OP;
struct rdma_request
{
    RDMA_OP rw;
    u64 local_dma_addr;
    u64 remote_dma_addr;
    int size;
};
struct rdfs_req_id
{
     u64 max_req_id;
     unsigned long lock_word;
     spinlock_t req_id_lock;
};


struct rdfs_message
{
   int m_type;
   char m_data[MAX_MESSAGE_LENGTH];
};

#endif
