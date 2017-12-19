/* @author page
 * @date 2017/12/3
 * rdfs/debug.h
 */

#ifndef _rdfs_DEBUG_H
#define _rdfs_DEBUG_H

#include <linux/buffer_head.h>
#include <linux/types.h>
#include <linux/crc32.h>
#include <linux/mutex.h>
#include <linux/spinlock_types.h>
#include <linux/rwlock.h>

#include "rdfs.h"
#define rdfs_info(s, args...)		pr_info(s, ## args)
#define rdfs_trace()                printk(KERN_INFO"%s:%s\n",__FILE__,__FUNCTION__)
#define rdfs_dbg(s, args...)		pr_debug(s, ## args)
#define rdfs_warn(s, args...)		pr_warning(s, ## args)
#define rdfs_err(sb, s, args...)	rdfs_error_mng(sb, s, ## args)


#define clear_opt(o, opt)	(o &= ~HMMM_MOUNT_##opt)
#define set_opt(o, opt)		(o |= RDFS_MOUNT_##opt)
#define test_opt(sb, opt)	(RDFS_SB(sb)->s_mount_opt & RDFS_MOUNT_##opt)


extern void rdfs_error(struct super_block * sb, const char * function,
            const char * fmt, ...);

extern void rdfs_msg(struct super_block * sb, const char * prefix,
            const char * fmt, ...);
            int rdfs_check_pointer(void *p);
#endif