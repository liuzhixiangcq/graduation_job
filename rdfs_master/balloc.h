/* @author page
 * @date 2017/12/3
 * rdfs/balloc.h
 */

 #ifndef _RDFS_BALLOC_H
 #define _RDFS_BALLOC_H
 
 #include <linux/io.h>
 #include <linux/mm.h>
 #include <asm-generic/bitops/le.h>
 #include <linux/spinlock_types.h>
 #include <linux/types.h>
 #include <linux/fs.h>
 #include <linux/genhd.h>
 
 #include <linux/kthread.h>
 #include <linux/delay.h>
 #include <linux/sched.h>
 
 #include <asm/pgtable.h>
 #include <asm/tlbflush.h>
 #include <asm/bug.h>
 #include <asm/paravirt.h>
 #include "rdfs.h"

 sector_t rdfs_get_bdev_size(struct block_device* bdev);
 size_t rdfs_init_bdev(struct rdfs_sb_info *sbi,char* path,unsigned long * bdev_address);
 void bdev_insert_a_page(unsigned long address);
 void rdfs_bindcore(unsigned long core_bits);
 void rdfs_bind_cpu_by_ino(int ino);
 void print_pgtable(struct mm_struct* mm,unsigned long address);
 void unexpected_do_fault(unsigned long address);
 void rdfs_init_bdev_pool(struct super_block* sb);
 unsigned long rdfs_get_bdev(unsigned long sbi_addr, void **data);
 void rdfs_init_free_inode_list_offset(struct numa_des* numa_des_p);
 void rdfs_init_free_block_list_offset(struct numa_des* numa_des_p);
 int rdfs_init_numa(unsigned long start , unsigned long end , unsigned long percentage);
 struct numa_des* get_cpu_numa(void);
 struct numa_des* get_a_numa(void);
 struct numa_des* get_rdfs_inode_numa(struct rdfs_inode* inode);
 int rdfs_new_block(phys_addr_t *physaddr,int num,struct rdfs_inode* ni);
 void* rdfs_get_page(void);
 void* rdfs_get_zeroed_page(void);
 
 void rdfs_free_block(struct super_block *sb, phys_addr_t phys );
 unsigned long rdfs_count_free_blocks(struct super_block *sb);
 struct bdev_des* rdfs_alloc_bdev(unsigned long * bdev_id);
 sector_t __rdfs_alloc_a_disk_block(struct bdev_des* bdev_des);
 sector_t rdfs_alloc_a_disk_block(struct bdev_des* bdev_des);
 unsigned long rdfs_free_a_disk_block(struct bdev_des* bdev_des, sector_t sn);
 void rdfs_pre_free_init(struct pre_free_struct* pre_free_struct);
 void rdfs_pre_free_block(struct pre_free_struct* pre_free_struct,phys_addr_t phys);
 void rdfs_pre_free_list(struct super_block* sb , struct pre_free_struct* pre_free_struct);
 
 #endif