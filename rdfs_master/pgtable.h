/* @author page
 * @date 2017/12/3
 * rdfs/pgtable.h
 */

 #ifndef _RDFS_PGTABLE_H
 #define _RDFS_PGTABLE_H

 #include <linux/fs.h>
 #include <linux/types.h>
 #include <linux/pagemap.h>
 #include <linux/mm.h>
 #include <linux/sched.h>
 #include <linux/init.h>
 #include <linux/kallsyms.h>
 #include <linux/mm_types.h>
 #include <linux/kernel.h>
 #include "rdfs.h"

 void rdfs_rm_pg_table(struct super_block *sb, u64 ino);
 pud_t *rdfs_pud_alloc(struct super_block *sb, unsigned long ino, unsigned long offset);
 pmd_t *rdfs_pmd_alloc(struct super_block *sb, pud_t *pud, unsigned long addr);
 pte_t *rdfs_pte_alloc(struct super_block *sb, pmd_t *pmd, unsigned long addr);
 int rdfs_init_pg_table(struct super_block * sb, u64 ino);
 int rdfs_insert_page(struct super_block *sb, struct inode *vfs_inode, phys_addr_t phys);
 void rdfs_rm_pte_range(struct super_block *sb, struct rdfs_inode* nv_i, pmd_t *pmd);
 void rdfs_rm_pmd_range(struct super_block *sb,struct rdfs_inode* nv_i, pud_t *pud);
 int rdfs_establish_mapping(struct inode *inode);
 int rdfs_destroy_mapping(struct inode *inode);
 int rdfs_search_metadata(struct rdfs_inode_info * ri_info,unsigned long offset,unsigned long *s_id,unsigned long *block_id);
 int rdfs_client_search_metadata(struct client_request_task*task,int ino,long offset,long length);
 #endif