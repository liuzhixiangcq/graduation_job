#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/un.h>
#include <linux/net.h>
#include <net/sock.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>

#include "ctx.h"
#include "init.h"
#include "event.h"
#include "rdfs.h"
#include "rdfs_sock.h"
#include "memory.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LZX zhixiangliucq@gmail.com");
MODULE_DESCRIPTION("rdfs rdma module");

int rdfs_register(void * ip_info)
{
    printk("%s\n",__FUNCTION__);

    struct ip_info * ip_info_p = (struct ip_info *)ip_info;
    int retval;
    struct sockaddr_in server_addr;
    struct socket * client_sock;
    struct rdfs_init_context * ctx_p;
    if(ip_info_p == NULL)
    {
        printk("%s --> args:ip_info is NULL\n",__FUNCTION__);
        return -1;
    }
   
    
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ip_info_p->port);
    server_addr.sin_addr.s_addr = in_aton(ip_info_p->ip);

    printk("%s --> master information:ip_addr:%s port:%d\n",__FUNCTION__,ip_info_p->ip,ip_info_p->port); 

    client_sock = (struct socket*)kmalloc(sizeof(struct socket),GFP_KERNEL);
    retval = sock_create_kern(AF_INET,SOCK_STREAM,IPPROTO_TCP,&client_sock);
    if(retval)
    {
        printk("%s --> sock_create_kern failed\n",__FUNCTION__);
        kfree(client_sock);
        return -1;
    }
    retval = kernel_connect(client_sock,(struct sockaddr*)&server_addr,sizeof(server_addr),0);

    if(retval)
    {
        printk("%s --> kernel_connect failed\n",__FUNCTION__);
        kfree(client_sock);
        return -1;
    }
   
    ctx_p = rdfs_init_context(ip_info_p->ib_dev,client_sock);
    if(ctx_p == NULL)
    {
        printk("%s --> rdfs_init_context failed\n",__FUNCTION__);
        //rdfs_destroy_context();
        kfree(client_sock);
        return -1;
    }
    return 0;

}

int rdfs_init_register_service(const char *ip,int server_port,struct ib_device * dev)
{
    printk("%s\n",__FUNCTION__);
    struct ip_info* arg = (struct ip_info*)kmalloc(sizeof(struct ip_info),GFP_KERNEL);
    strcpy(arg->ip,ip);
    arg->port = server_port;
    arg->ib_dev = dev;
    //printk("%s --> ip_info.port:%lx\n",__FUNCTION__,ip_info.port);
    struct task_struct * rdfs_register_service = kthread_run(rdfs_wait_register,(void*)arg,"rdfs_register_service");
    if(IS_ERR(rdfs_register_service))
    {
        printk("%s --> rdfs register service start failed\n",__FUNCTION__);
        return -1;
    }
    else
    {
        printk("%s rdfs register service start ok\n",__FUNCTION__);
        return 0;
    }
}

static void rdfs_add_device(struct ib_device * dev)
{
	printk("%s\n",__FUNCTION__);

    int retval;
    struct ip_info ip_info;

    struct ib_event_handler ieh;
    
    const char * master_ip = MASTER_IP;
    int master_port = MASTER_REGISTER_PORT;
    
    strcpy(ip_info.ip,master_ip);
    ip_info.port = master_port;
    ip_info.ib_dev = dev;

    // ***********************//
    INIT_IB_EVENT_HANDLER(&ieh,dev,async_event_handler);
    ib_register_event_handler(&ieh);

    // ***********************//
    retval = rdfs_register((void*)&ip_info);

    if(retval)
    {
        printk("%s --> rdfs_register failed\n",__FUNCTION__);
        return;
    }

    const char * slave_ip = SLAVE_IP;
    int slave_port = SLAVE_REGISTER_PORT;
    
    // ***********************//
    retval = rdfs_init_register_service(slave_ip,slave_port,dev);
    if(retval)
    {
        printk("%s --> rdfs_init_register_service failed\n",__FUNCTION__);
        return;
    }
}

static void rdfs_remove_device(struct ib_device * dev)
{
    printk("%s\n",__FUNCTION__);
    int retval;
    //rdfs_exit_slave();
    //rdfs_unregister();
    //rdfs_close_register_service();

}

static struct ib_client slave_ib_client=
{
    .name = "rdfs test module",
    .add = rdfs_add_device,
    .remove = rdfs_remove_device
};

/* init slave module*/

static int __init rdfs_init(void)
{
    printk("%s\n",__FUNCTION__);
    ib_register_client(&slave_ib_client);

    return 0;
}

/* exit slave module*/

static void __exit rdfs_exit(void)
{
	printk("%s\n",__FUNCTION__);

    ib_unregister_client(&slave_ib_client);
}

module_init(rdfs_init);
module_exit(rdfs_exit);
