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

  struct mem_bitmap
  {
      unsigned long map[BITMAP_ARRAY_LEN];
      spin_lock_t map_lock;
      int flag;//whether has free block
  };
  struct slave_info
  {
     struct rdfs_context * ctx;
     struct ib_device * dev ;
     int slave_id;
     int slave_status;
     int free_block_nums;
     int bitmap_array_nums;
     spin_lock_t slave_lock;
     struct mem_bitmap **bitmap;
  };
  
 #endif
