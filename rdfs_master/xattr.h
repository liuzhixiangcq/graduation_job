/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */

#ifndef _RDFS_XATTR_H
#define _RDFS_XATTR_H

#include <linux/init.h>
#include <linux/xattr.h>

#include "debug.h"

#ifdef CONFIG_RDFS_XATTR

#else 

static inline int rdfs_xattr_get(struct inode *inode, int name_index,const char *name, void *buffer, size_t size)
{
	rdfs_trace();
	return -EOPNOTSUPP;
}

static inline int rdfs_xattr_set(struct inode *inode, int name_index, const char *name,const void *value, size_t size, int flags)
{
	rdfs_trace();
	return -EOPNOTSUPP;
}

static inline void rdfs_xattr_delete_inode(struct inode *inode){}

static inline void rdfs_xattr_put_super(struct super_block *sb){}

static inline int init_rdfs_xattr(void)
{
	return 0;
}

static inline void exit_rdfs_xattr(void){}

#define rdfs_xattr_handlers NULL

#endif

#ifdef CONFIG_rdfsFS_SECURITY
extern int rdfs_init_security(struct inode *inode, struct inode *dir, const struct qstr *qstr);
#else
static inline int rdfs_init_security(struct inode *inode, struct inode *dir, const struct qstr *qstr)
{
	rdfs_trace();
	return 0;
}
#endif

#endif


