/* @author page
 * @date 2017/12/3
 * rdfs/nvmalloc.h
 */

 #ifndef _RDFS_NVMALLOC_H
 #define _RDFS_NVMALLOC_H

 #include <linux/mm.h>
 #include <linux/module.h>
 #include <linux/sched.h>
 #include <linux/kallsyms.h>
 #include <linux/highmem.h>
 #include <linux/slab.h>
 #include <linux/vmalloc.h>
 #include "rdfs.h"
 #include <asm/pgalloc.h>
 #include <asm/pgtable.h>
 #include <asm/tlbflush.h>
 #include <linux/rcupdate.h>
 #include <linux/spinlock.h>
 
 #include "rdfs.h"

 int unnvmap(unsigned long addr, pud_t *ppud, struct mm_struct* mm);
 void *nvmalloc(const int mode);
 void nvfree(const void *addr);
 int nvmap_pmd(struct super_block* sb, const unsigned long addr, pmd_t *ppmd, struct mm_struct *mm);
 int nvmap_dir(struct super_block* sb, unsigned long addr, pud_t *ppud, struct mm_struct *mm);
 int nvmap_file(struct super_block* sb, unsigned long addr, pud_t *ppud, struct mm_struct *mm);
 int unnvmap_file(unsigned long addr, pud_t *ppud, struct mm_struct *mm);
 int nvmap(struct super_block* sb, unsigned long addr, pud_t *ppud, struct mm_struct *mm);
 int unnvmap_dir(unsigned long addr, struct mm_struct *mm);
 int nvmalloc_init(void);

 #endif