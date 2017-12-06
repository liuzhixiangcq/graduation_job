/* @author page
 * @date 2017/12/3
 * rdfs/acl.h
 */
#ifndef _RDFS_ACL_H
#define _RDFS_ACL_H
#include <linux/posix_acl_xattr.h>

#include "rdfs.h"

#define rdfs_ACL_VERSION   0x0001

struct rdfs_acl_entry {
	__be16		e_tag;
	__be16		e_perm;
	__be32		e_id;
};

struct rdfs_acl_entry_short {
	__be16		e_tag;
	__be16		e_perm;
};

struct rdfs_acl_header {
	__be32		a_version;
};

int rdfs_acl_chmod(struct inode *inode);
int rdfs_init_acl(struct inode *inode, struct inode *dir);
struct posix_acl *rdfs_get_acl(struct inode *inode, int type);

#endif
