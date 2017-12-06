/* @author page
 * @date 2017/12/3
 * rdfs/dir.h
 */

 #ifndef _RDFS_DIR_H
 #define _RDFS_DIR_H

 #include <linux/sched.h>
 #include <linux/fs.h>
 #include <linux/types.h>
 #include <linux/pagemap.h>
 #include <linux/init.h>
 #include <linux/mm.h>
 #include <linux/mm_types.h>
 #include <linux/kernel.h>
 #include <linux/kallsyms.h>
 #include <linux/list.h>
 #include <asm/page.h>
 #include "rdfs.h"
 
 struct rdfs_dir_entry *rdfs_find_entry2(struct inode *dir, struct qstr *child, struct rdfs_dir_entry **up_nde);
 ino_t rdfs_inode_by_name(struct inode *dir,struct qstr *child);
 void rdfs_set_link(struct inode *dir, struct rdfs_dir_entry *de, struct inode *inode,int update_times);
 int rdfs_add_link(struct dentry *dentry,struct inode *inode);
 int rdfs_delete_entry(struct rdfs_dir_entry *dir,struct rdfs_dir_entry **pdir,struct inode *parent);
 int rdfs_make_empty(struct inode *inode,struct inode *parent);
 struct rdfs_dir_entry *rdfs_dotdot(struct inode *dir);
 int rdfs_empty_dir(struct inode *inode);

 #endif