/* @author page
 * @date 2017/12/3
 * rdfs/tool.h
 */
#ifndef _RDFS_TOOL_H
#define _RDFS_TOOL_H

 #include <linux/module.h>
 #include <linux/init.h>
 #include <linux/kernel.h>
 #include <linux/string.h>
 #include <linux/fs.h>
 #include <linux/slab.h>
 #include <linux/init.h>
 #include <linux/blkdev.h>

 #include <linux/random.h>
 #include <linux/buffer_head.h>
 #include <linux/exportfs.h>
 #include <linux/vfs.h>
 #include <linux/seq_file.h>
 #include <linux/mount.h>
 #include <linux/mm.h>
 #include <linux/kthread.h>
 #include <linux/delay.h>
 #include "rdfs.h"
 void rdfs_get_init_size(struct rdfs_sb_info *sbi, void **data);
 void init_address_to_ino(unsigned long address);
 void rdfs_set_blocksize(struct super_block *sb, unsigned long size);
 void set_default_opts(struct rdfs_sb_info *sbi);
 loff_t rdfs_max_size(int bits);
 int isdigit(int ch);
 int rdfs_swap_buffer_free(struct rdfs_inode_info *ni_info);
 phys_addr_t get_phys_addr(void **data);
 int calc_super_checksum(void);
 int rdfs_ioremap(struct super_block *sb, unsigned long size);
 int rdfs_iounmap(struct super_block *sb,unsigned long size);
 void rdfs_i_callback(struct rcu_head *head);
 void init_once(void *foo);
 int init_inodecache(void);
 void destory_inodecache(void);
 int rdfs_calc_checksum(u8 *data, int n);
 int rdfs_parse_options(char *options, struct rdfs_sb_info *sbi,bool remount);
 #endif