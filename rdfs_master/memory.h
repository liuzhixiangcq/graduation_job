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

  #define ALLOC_PAGE 1
  #define ALLOC_PTE  2

  #define SLAVE_ID_SHIFT 51
  #define SLAVE_ID_MASK  0x000fffffffffffff

  struct slave_info
  {
     struct rdfs_context * ctx;
     struct ib_device * dev ;
     int slave_id;
     int status;

     int free_block_nums;
     int total_block_nums;
     
     spinlock_t slave_free_list_lock;
     //struct mem_block * head;
     struct mem_bitmap_block *bitmap_head;
     struct socket *slave_sock;
  };
  struct pte_mem
  {
        unsigned long pte_start_addr;
        int free_pte_nums;
        int total_page_nums;
        spinlock_t pte_lock;
        int start_addr;
  };
  struct mem_bitmap_block
  {
    unsigned long slave_id;
    unsigned long start_block_id;
    struct mem_bitmap_block * next;
  };
  struct pre_alloc_bitmap
  {
    unsigned long pre_alloc_bitmap;
    unsigned long pre_start_block_id;
    int pre_alloc_slave_id;
    int pre_index;
    spinlock_t bitmap_lock;
  };
  int rdfs_new_pte(struct rdfs_inode_info * ri_info,unsigned long *phyaddr);
  int rdfs_init_slave_memory_bitmap_list(struct slave_info *s);
  int rdfs_free_slave_memory_bitmap_list(struct slave_info *s);
 #endif
