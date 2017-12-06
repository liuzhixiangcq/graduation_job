/* @author page
 * @date 2017/12/1
 */

 #ifndef _EVENT_H
 #define _EVENT_H

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

#include "init.h"
#include "ctx.h"

void async_event_handler(struct ib_event_handler* ieh,struct ib_event* ie);
void comp_handler_send(struct ib_cq* cq,void *cq_context);
void comp_handler_recv(struct ib_cq *cq,void * cq_context);
void cq_event_handler_send(struct ib_event* ib_e,void * cq_context);
void cq_event_handler_recv(struct ib_event* ib_e,void * cq_context);

#endif