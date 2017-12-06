/* @author page
 * @date 2017/12/3
 * rdfs/debug.c
 */

 
#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include <linux/mutex.h>
#include <linux/spinlock_types.h>
#include <linux/rwlock.h>

#include "rdfs.h"

void rdfs_error(struct super_block * sb, const char * function,
    const char * fmt, ...)
{
va_list args;

va_start(args, fmt);
printk(KERN_ERR "rdfsFS (%s): error: %s: ", sb->s_id, function);
vprintk(fmt, args);
printk("\n");
va_end(args);
}
void rdfs_debug_msg(const char* function,const char* fmt,...)
{
    val_list_args;
    va_start(args,fmt);
    printk("%s:\n",function);
    vprintk(fmt,args);
    printk("\n");
    va_end(args);
}
void rdfs_msg(struct super_block * sb, const char * prefix,
    const char * fmt, ...)
{
va_list args;

va_start(args, fmt);
printk("%srdfsFS (%s): ", prefix, sb->s_id);
vprintk(fmt, args);
printk("\n");
va_end(args);
}
int rdfs_check_pointer(void *p)
{
    if(p == NULL)
    {
        printk("%s --> pointer is NULL\n",__FUNCTION__);
        return 1;
    }
    else
        return 0;
}