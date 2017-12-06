/* @author page
 * @date 2017/12/3
 * rdfs/super.c
 */
 #ifndef _RDFS_INODE_H
 #define _RDFS_INODE_H

 #include <linux/time.h>
 #include <linux/fs.h>
 #include <linux/hardirq.h>
 #include <linux/sched.h>
 #include <linux/highuid.h>
 #include <linux/quotaops.h>
 #include <linux/sched.h>
 #include <linux/mpage.h>
 #include <linux/backing-dev.h>

 #include "rdfs.h"
 

//extern struct address_space_operations rdfs_aops;

u64 rdfs_find_data_block(struct inode *inode, unsigned long file_blocknr);
void rdfs_truncate_blocks(struct inode *inode, loff_t start, loff_t end);
int rdfs_alloc_blocks(struct inode *inode, int num, int zero);
int rdfs_update_inode(struct inode *inode);
void rdfs_set_inode_flags(struct inode *inode);
void get_rdfs_inode_flags(struct rdfs_inode_info *ei);
struct inode *rdfs_iget(struct super_block *sb, unsigned long ino);
struct inode *rdfs_new_inode(struct inode *dir, umode_t mode, const struct qstr *qstr);
int rdfs_notify_change(struct dentry *dentry, struct iattr *attr);
int __rdfs_write_inode(struct inode *inode, int do_sync);
void rdfs_free_inode(struct inode *inode);

#endif