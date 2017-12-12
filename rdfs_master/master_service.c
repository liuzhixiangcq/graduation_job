/* @author page
 * @date 2017/12/5
 * rdfs_master/master_service.c
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

 struct task_struct * rdfs_slave_register_service;
 struct task_struct * rdfs_client_request_service;
 static int slave_register_service_stop_flag = 0;
 static int client_request_service_stop_flag = 0;
 
 struct slave_info slave_infos[MAX_SLAVE_NUMS];

 static int slave_id = 0;

 static int rdfs_process_register(struct service_info* s_info)
 {
    rdfs_trace();
    struct rdfs_context * ctx_p = rdfs_init_context(s_info->dev);
    if(ctx_p == NULL)
    {
        rdfs_debug_msg(__FUNCTION__,"rdfs_init_context failed\n");
        return -1;
    }
    int retval;
    int m_type;
    retval = rdfs_recv_message(s_info->sock.c_sock,ctx_p,&m_type);
    retval = rdfs_send_message(s_info->sock.c_sock,ctx_p,MASTER_CTX_INFO);
    
    
    slave_infos[slave_id].slave_id = slave_id;
    slave_infos[slave_id].ctx = ctx_p;
    slave_infos[slave_id].dev = s_info->dev;
    slave_infos[slave_id].slave_status = SLAVE_ALIVE;
    slave_infos[slave_id].slave_sock = (s_info->sock).c_sock;
    spin_lock_init(&slave_infos[slave_id].slave_free_list_lock);
    rdfs_init_slave_memory_list(&slave_infos[slave_id]);
    slave_id ++;
    return 0;
 }
 static int rdfs_remove_register(struct slave_info* slave)
 {
    rdfs_free_slave_memory_list(slave);
    rdfs_remove_context(slave->ctx,slave->dev);
    slave->status = SLAVE_REMOVED;
    return 0;
 }
 static int rdfs_process_request(struct host_info* host_p)
 {
    return 0;
 }
 static int rdfs_init_service(void * arg)
 {
     rdfs_trace();
   
     struct service_info s_info;
     struct service_info * s_info_p = (struct service_info*)arg;
     
     strcpy(s_info.ip,s_info_p->ip);
     s_info.port = s_info_p->port;
     s_info.dev = s_info_p->dev;
     s_info.service_type = s_info_p->service_type;
     s_info.sock.s_sock = NULL;
     s_info.sock.c_sock = NULL;
     int retval;
     struct sockaddr_in server_addr;

     set_current_state(TASK_UNINTERRUPTIBLE);

     if(kthread_should_stop())
     {
         return -1;
     }

     memset(&server_addr,0,sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(s_info.port);
     server_addr.sin_addr.s_addr = in_aton(s_info.ip);

     s_info.sock.s_sock = (struct socket*)kmalloc(sizeof(struct socket),GFP_KERNEL);
     s_info.sock.c_sock = (struct socket*)kmalloc(sizeof(struct socket),GFP_KERNEL);

     retval = sock_create_kern(AF_INET,SOCK_STREAM,IPPROTO_TCP,&(host.sock).s_sock);

     if(retval)
     {
         return -1;
     }
     retval = kernel_bind((s_info.sock).s_sock,(struct sockaddr*)&server_addr,sizeof(struct sockaddr_in));
     if(retval)
     {
         return -1;
     }
     retval = kernel_listen((s_info.sock).s_sock,SERVER_SOCK_LISTEN_NUMS);
     if(retval)
     {
         return -1;
     }
     while(1)
     {
         retval = kernel_accept((s_info.sock).s_sock,&(s_info.sock).c_sock,0);
         if(retval)
         {
             continue;
         }
         if(s_info.service_type == SLAVE_REGISTER_SERVICE)
         {
             retval = rdfs_process_register(&s_info);
             if(slave_register_service_stop_flag)break;
         }
         else if(s_info.service_type == CLIENT_REQUEST_SERVICE)
         {
             retval = rdfs_process_request(&s_info);
             if(client_request_service_stop_flag)break;
         }
     }
     sock_release(s_info.sock.s_sock);
     sock_release(s_info.sock.c_sock);
     kfree(s_info.sock.s_sock);
     kfree(s_info.sock.c_sock);
     return 0;
 
 }
 int rdfs_init_slave_register_service(const char *ip,int port,struct ib_device * dev)
 {
     rdfs_trace();
     struct service_info info;

     strcpy(info.ip,ip);
     info.port = port;
     info.ib_dev = dev;
     info.service_type = SLAVE_REGISTER_SERVICE;
     info.sock.s_sock = NULL;
     info.sock.c_sock = NULL;
    
     rdfs_slave_register_service = kthread_run(rdfs_init_service,(void*)&info,"rdfs_wait_slave_register_service");
     if(IS_ERR(rdfs_slave_register_service))
     {
         rdfs_debug_msg(__FUNCTION__,"slave_register_service init success");
         return -1;
     }
     else
     {
        rdfs_debug_msg(__FUNCTION__,"slave_register_service init failed");
         return 0;
     }
 }
 int rdfs_exit_service(struct task_struct* task)
 {
     if(IS_ERR(task))
     {
         rdfs_debug_msg(__FUNCTION__,"task not running");
         return -1;
     }
     if(task == slave_register_service)slave_register_service_stop_flag = 1;
     else client_request_service_stop_flag = 1;
     return 0;
 }
 int rdfs_init_client_request_service(const char *ip,int port,struct ib_device * dev)
 { 
     rdfs_trace();
     struct service_info info;
     
     strcpy(info.ip,ip);
     info.port = port;
     info.ib_dev = dev;
     info.service_type = SLAVE_REGISTER_SERVICE;

     rdfs_client_request_service = kthread_run(rdfs_init_service,(void*)&info,"rdfs_wait_client_request_service");
     if(IS_ERR(rdfs_client_request_service))
     {
        rdfs_debug_msg(__FUNCTION__,"client_request_service init success");
         return -1;
     }
     else
     {
        rdfs_debug_msg(__FUNCTION__,"client_request_service init failed");
         return 0;
     }
 }