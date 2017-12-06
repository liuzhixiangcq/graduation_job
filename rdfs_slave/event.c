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
#include "event.h"


void async_event_handler(struct ib_event_handler* ieh,struct ib_event* ie)
{
    printk("%s\n",__FUNCTION__);
}
void comp_handler_send(struct ib_cq* cq,void *cq_context)
{
    struct ib_wc wc;
    printk("%s\n",__FUNCTION__);
   
    do 
    {
        while(ib_poll_cq(cq,1,&wc)>0)
        {
            if(wc.status == IB_WC_SUCCESS)
            {
                printk("%s IB_WC_SUCCESS WR_ID:%llx\n",__FUNCTION__,wc.wr_id);
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
    struct ib_wc wc;
    printk("%s\n",__FUNCTION__);
    
    do 
    {
        while(ib_poll_cq(cq,1,&wc)>0)
        {
            if(wc.status == IB_WC_SUCCESS)
            {
                printk("%s IB_WC_SUCCESS\n",__FUNCTION__);
            }
            else
            {
                printk("%s FAILURE %d\n",__FUNCTION__,wc.status);
            }
        }
    }while(ib_req_notify_cq(cq,IB_CQ_NEXT_COMP|IB_CQ_REPORT_MISSED_EVENTS)>0);
    
}
void cq_event_handler_send(struct ib_event* ib_e,void * cq_context)
{
    printk("%s \n",__FUNCTION__);
}
void cq_event_handler_recv(struct ib_event* ib_e,void * cq_context)
{
    printk("%s \n",__FUNCTION__);
}