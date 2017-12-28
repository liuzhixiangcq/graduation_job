/* @author page
 * @date 2017/12/25
 */

 #ifndef _RDFS_NETWORK_H
 #define _RDFS_NETWORK_H
 #include <infiniband/verbs.h>
 #include "network.h"
 #include "rdfs_config.h"
 #include "rdfs.h"
 #include <stdio.h>
 #include <unistd.h>
 #include <stdint.h>
 #include <stdlib.h>
 #include <string.h>
 #include <sys/types.h>
 #include <sys/ipc.h>
 #include <sys/shm.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <fcntl.h>
 #define CLIENT_CTX_INFO_TO_MASTER  1
 #define CLIENT_CTX_INFO_TO_SLAVE   2
 #define CLIENT_SERACH_SLAVE_INFO   3
 #define SLAVE_CTX_TO_CLIENT        6
 #define SLAVE_CTX_INFO_TO_CLIENT   4
 #define MASTER_CTX_INFO_TO_CLIENT  5

 
 struct slave_context 
 {
     struct ibv_context* context;
     struct ibv_pd* pd;
     struct ibv_mr* mr;
     struct ibv_qp* qp;
     struct ibv_cq* send_cq;
     struct ibv_cq* recv_cq;
     struct ibv_comp_channel *event_channel;
  
     int active_mtu;   
     uint32_t rkey;  // rkey to be sent to client
     //union ibv_gid gid;
     int qpn;
     int psn;
     int lid;
  
     //int ino;
  
     int rem_qpn;
     int rem_psn;
     int rem_lid;
     unsigned long long int rem_vaddr;
     uint32_t rem_rkey;
     int rem_block_nums;
     char* rdma_buffer;
    
     unsigned long dma_addr;
     unsigned long rdma_mem_size; 
  
     //pthread_t cq_poller_thread;
  };
 
  struct rdfs_message
  {
     int m_type;
     char m_data[MAX_MESSAGE_LENGTH];
     int nums;
  };
  struct rdfs_search_message
  {
     int m_type;
     int nums;
     long info[4][MAX_MESSAGE_LENGTH];
  };
  enum RDFS_CLIENT_REQUEST
  {
      RDFS_CLIENT_OPEN,
      RDFS_CLIENT_CLOSE,
      RDFS_CLIENT_MKDIR,
      RDFS_CLIENT_RMDIR,
      RDFS_CLIENT_LISTDIR,
      RDFS_CLIENT_READ,
      RDFS_CLIENT_WRITE
  };
  int rdfs_connect(unsigned int ip,int port);
  int rdfs_send_message(int sock_fd,struct slave_context* ctx_p,int m_type);
  int rdfs_recv_message(int sock_fd,struct slave_context* ctx_p,int *m_type);
  int send_data(int fd,void* data,int size);
  int recv_data(int fd,void* data,int size);
 #endif