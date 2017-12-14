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
  struct mem_block
  {
    int block_id;
    struct mem_block *next;
  };
  struct slave_info
  {
     struct rdfs_context * ctx;
     struct ib_device * dev ;
     int slave_id;
     int slave_status;

     int free_block_nums;
     int toal_block_nums;
     
     spin_lock_t slave_free_list_lock;
     struct mem_block * head;
     struct mem_bitmap_block *bitmap_head;
     struct socket *slave_sock;
  };
  struct pte_mem
  {
        unsigned long pte_start_addr;
        int free_pte_nums;
        int total_page_nums;
        spin_lock_t pte_lock;
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
    spin_lock_t bitmap_lock;
  };
 #endif
