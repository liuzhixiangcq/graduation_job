/* @author page
 * @date 2017/12/3
 * rdfs/memory.h
 */

 #ifndef _RDFS_MEMORY_H
 #define _RDFS_MEMORY_H

 #include <linux/fs.h>
 #include <linux/sched.h>
 #include <linux/aio.h>
 #include <linux/slab.h>
 #include <linux/uio.h>
 #include <linux/mm.h>
 #include <linux/uaccess.h>
 
 #include <linux/delay.h>
 #include <linux/sched.h>
 #include <linux/bio.h>
 #include <linux/rwlock.h>
 
 #include <asm/barrier.h>
 #include <asm/tlbflush.h>
 
  #include<linux/fs.h>
  #include<linux/kernel.h>
  #include<linux/init.h>
  #include<asm/unistd.h>
  #include<asm/uaccess.h>
  #include<linux/types.h>
  #include<linux/fcntl.h>
  #include<linux/dirent.h>
  #include<linux/kdev_t.h>
  #include "rdfs.h"

  #define FK_SIZE  (1<<12)
  #define TM_SIZE  (1<<21)
  #define OTM_SIZE (1<<27)
  #define OG_SIZE  (1<<30)


typedef struct slave_test
{
	unsigned long slave_id;
	unsigned long mem_size;
	unsigned long phy_addr;
}slave_test;

typedef struct mem_node
{
	unsigned long cur_addr;
	struct mem_node *pre_node;
	struct mem_node *next_node;
	unsigned long mem_size;
	unsigned long slave_id;
}mem_node;

typedef struct slave_info
{
	unsigned long mem_size;
	unsigned long phy_addr;
	unsigned long slave_id;
	struct mem_node **free_list;
	unsigned long cur_mem_size;
}slave_info;

struct index_struct
{
    unsigned long  sum;
    struct mem_node *p;
};

struct index_search_result
{
    unsigned long  slave_id;
    unsigned long  start_addr;
    unsigned long  size;
    unsigned long  block_type;
    unsigned long  file_offset;
};

  unsigned long path_to_inode(char *path);
  void rdfs_init_mem_node(struct mem_node *head_node,unsigned long  size);
  void rdfs_add_mem_to_list(unsigned long slave_id,unsigned long index,unsigned long size,unsigned long phy_addr);
  unsigned long rdfs_split_mem_from_toplist(unsigned long slave_id,unsigned long index);
  void rdfs_split_mem(unsigned long slave_id,unsigned long smallpersent,unsigned long largepersent);
  void rdfs_init_freelist(unsigned long slave_id,unsigned long mem_size,unsigned long phy_addr);
  unsigned long rdfs_get_new_block(unsigned long slave_id,unsigned long size,struct mem_node *block);
  unsigned long rdfs_new_block_update(unsigned long size,struct mem_node *block);
  void rdfs_free_blcok(unsigned long size,unsigned long phy_addr,unsigned slave_id);
  void dmfs_index_init(struct rdfs_inode_info *inode);
  void dmfs_index_free(struct rdfs_inode_info *inode);
  void dmfs_index_update(struct rdfs_inode_info *inode);
  int dmfs_new_block(struct rdfs_inode_info *inode,unsigned long size);
  unsigned long dmfs_block_search(struct rdfs_inode_info *inode,unsigned long key);
  unsigned long dmfs_index_search(struct rdfs_inode_info *inode,loff_t offset,unsigned long size,struct index_search_result *result);
 #endif
