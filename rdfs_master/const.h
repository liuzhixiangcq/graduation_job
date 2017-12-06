/* @author page
 * @date 2017/12/3
 * rdfs/const.h
 */

#ifndef _RDFS_CONST_H
#define _RDFS_CONST_H

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
 #define NVMM_MIN_BLOCK_SIZE       (PAGE_SIZE)
 #define NVMM_MAX_BLOCK_SIZE       (2 * (PAGE_SIZE))


#define RDFS_SUPER_MAGIC 0xEFFB


#endif