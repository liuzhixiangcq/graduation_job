/* @author page
 * @date 2017/12/6
 * rdfs_master/device.c
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
 #include "rdfs_config.h"
 #include "master_service.h"
 static void rdfs_add_device(struct ib_device* dev)
 {
     rdfs_trace();
     int retval;
     const char *master_ip = MASTER_IP;
     int slave_register_port = SLAVE_REGISTER_PORT;
     int client_request_port = CLIENT_REQUEST_PORT;

     INIT_IB_EVENT_HANDLER(&ieh,dev,async_event_handler);
     ib_register_event_handler(&ieh);

     retval = rdfs_init_slave_register_service(master_ip,slave_register_port,dev);
     if(retval)
     {
         rdfs_debug_msg(__FUNCTION__,"rdfs_init_slave_register_service failed");
         return;
     }
     
     retval = rdfs_init_client_request_service(master_ip,client_request_port,dev);
     if(retval)
     {
         rdfs_debug_msg(__FUNCTION__,"rdfs_init_client_request_service failed");
         return;
     }
     return ;
 }
 static void rdfs_remove_device(struct ib_device* dev)
 {
     rdfs_trace();
     int retval;
     extern struct task_struct * rdfs_slave_register_service;
     extern struct task_struct * rdfs_client_request_service;
     retval = rdfs_exit_service(rdfs_client_request_service);
     retval = rdfs_exit_service(rdfs_slave_register_service);
     ib_unregister_event_handler(&ieh);
 }
 
static struct ib_client master_ib_client=
{
    .name = "rdfs test module",
    .add = rdfs_add_device,
    .remove = rdfs_remove_device
};

int rdfs_init(void)
{
    rdfs_trace();
    ib_register_client(&master_ib_client);
    return 0;
}
int rdfs_exit(void)
{
    rdfs_trace();
    ib_unregister_client(&master_ib_client);
     return 0;
}