/* @author page
 * @date 2017/12/3
 * rdfs/nvmalloc.c
 */


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
#include "debug.h"

#define DIR_AREA_SIZE       (1UL << 35) // 32G
#define MAX_DIR_SIZE        (1UL << 21) // 2M

#define MAX_FILE_SIZE       (1UL << 35) // 32G

#define NVMALLOC_DEBUG

struct nvm_struct 
{
    struct nvm_struct   *next;
    void                *vaddr;
    unsigned long       size;
};

static struct nvm_struct *dir_free_list = NULL;
static struct nvm_struct *dir_used_list = NULL;
static int dir_free_cnt = 0;
static int dir_used_cnt = 0;

static struct nvm_struct *reg_free_list = NULL;
static struct nvm_struct *reg_used_list = NULL;
static int reg_free_cnt = 0;
static int reg_used_cnt = 0;
static DEFINE_SPINLOCK(list_lock);



void *nvmalloc(const int mode)
{
	rdfs_trace();
    struct nvm_struct *p;
    struct nvm_struct *free_list, *used_list;
    int *used_cnt, *free_cnt;
    spin_lock(&list_lock);
    if (mode == 0) {            /* regular file */
        free_list = reg_free_list;
        used_list = reg_used_list;
        free_cnt  = &reg_free_cnt;
        used_cnt  = &reg_used_cnt;
    }
    else if (mode == 1) {       /* directory file */
        free_list = dir_free_list;
        used_list = dir_used_list;
        free_cnt  = &dir_free_cnt;
        used_cnt  = &dir_used_cnt;
    }
    else {
        spin_unlock(&list_lock);
        printk(KERN_WARNING "nvmalloc: unknown mode %d\n", mode);
        return NULL;
    }

    if (*free_cnt > 0) {
        p = free_list;
        free_list = free_list->next;

        if (used_list == NULL) { /* empty used list */
            used_list = p;
            used_list->next = NULL;
        }
        else {
            p->next = used_list;
            used_list = p;
        }
            
        (*used_cnt)++;
        (*free_cnt)--;
        goto out;
    }
    else {
        spin_unlock(&list_lock);
        printk(KERN_WARNING "nvmalloc: no free space!\n");
        return NULL;
    }

out:
    /* remember to update free_list and used_list !!!*/
    if (mode == 0) {
        reg_free_list = free_list;
        reg_used_list = used_list;
    }
    else {
        dir_free_list = free_list;
        dir_used_list = used_list;
    }
    spin_unlock(&list_lock);
    
    return p->vaddr;
}

void nvfree(const void *addr)
{
	rdfs_trace();
    struct nvm_struct *p;
    struct nvm_struct *pre;
    struct nvm_struct *free_list, *used_list;
    int *free_cnt, *used_cnt;
    int mode = -1;

    if(addr == NULL) {
        printk(KERN_ERR "nvfree: can not free a null address.\n");
        return;
    }


    /* remenber to lock it */
    spin_lock(&list_lock);

    /* directory file */
    if ( ((unsigned long)addr) >= RDFS_START &&
                ((unsigned long)addr) < RDFS_START + DIR_AREA_SIZE) {
        free_list = dir_free_list;
        used_list = dir_used_list;
        free_cnt  = &dir_free_cnt;
        used_cnt  = &dir_used_cnt;
        mode = 1;
    }
    else if ( ((unsigned long)addr) >= RDFS_START + DIR_AREA_SIZE &&
                ((unsigned long)addr) < RDFS_END) {  /* regular file */
        free_list = reg_free_list;
        used_list = reg_used_list;
        free_cnt  = &reg_free_cnt;
        used_cnt  = &reg_used_cnt;
        mode = 0;
    }
    else {
        spin_unlock(&list_lock);
        printk(KERN_WARNING "nvfree: unknown addr %lx\n", (unsigned long)addr);
        return;
    }

    /* walk through the used list, look for the addr */
    if (used_list->vaddr == addr) {

        p = used_list;  /* free p */
        used_list = used_list->next;
        
        /* add p to free list */
        p->next = free_list;
        free_list = p;

        (*free_cnt)++;
        (*used_cnt)--;

        goto out;
    }
    else {
        pre = used_list;
        for(p = used_list->next; p != NULL; p = p->next) {
            if (p->vaddr == addr) { /* OK, we got it */

                /* remove p from the used list */
                pre->next = p->next;

                /* add p to free list */
                p->next = free_list;
                free_list = p;

                (*free_cnt)++;
                (*used_cnt)--;

                goto out;
            }
            else 
                pre = p;
        }

        goto fail;
    }

fail:
    spin_unlock(&list_lock);
    /* Oops, we could't find the addr */
    printk(KERN_ERR "nvfree: free virtual address %lx faild, not used.\n", (unsigned long)addr);
    return;


out:
    /* remember to update free_list and used_list !!!*/
    if (mode == 0) {
        reg_free_list = free_list;
        reg_used_list = used_list;
    }
    else {
        dir_free_list = free_list;
        dir_used_list = used_list;
    }
    spin_unlock(&list_lock); 
}


void print_free_list(int mode)
{
    rdfs_trace();
    struct nvm_struct *p = NULL;
    if (mode == 0)
        p = reg_free_list;
    else
        p = dir_free_list;

    printk(KERN_INFO "***************nvmalloc:free list***************\n");
    if (p == NULL) printk(KERN_INFO "null\n");
    for(p = reg_free_list; p != NULL; p = p->next)
        printk(KERN_INFO "[%lx %lx]\n", (unsigned long)p->vaddr,(unsigned long)p->vaddr + p->size - 1);
}

void print_used_list(int mode)
{
    rdfs_trace();
    struct nvm_struct *p = NULL;
    if(mode == 0)
        p = reg_used_list;
    else
        p = dir_used_list;

    printk(KERN_INFO "***************nvmalloc:used list***************\n");
    if (p == NULL) printk(KERN_INFO "null\n");
    for(p = reg_used_list; p != NULL; p = p->next)
        printk(KERN_INFO "[%lx %lx]\n", (unsigned long)p->vaddr,
                    (unsigned long)p->vaddr + p->size - 1);

}

int nvmap_pmd(struct super_block* sb, const unsigned long addr, pmd_t *ppmd, struct mm_struct *mm)
{
    rdfs_trace();
    pgd_t *pgd;
    pud_t *pud;
	printk("%s\n",__FUNCTION__);
    if(!addr) {
        printk(KERN_WARNING "nvmap_pmd: null pointer error.\n");
        return -1;
    }
    pgd = pgd_offset(mm, addr);
    pud = pud_alloc(mm, pgd, addr);
    if(!pud)
        return -ENOMEM;

    rdfs_pud_populate(pud, ppmd);

    return 0;
}



int nvmap_dir(struct super_block* sb, unsigned long addr, pud_t *ppud, struct mm_struct *mm)
{
	rdfs_trace();
    unsigned long end = addr + MAX_DIR_SIZE - 1;
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;

    pgd = pgd_offset(mm, addr);
    pud = pud_alloc(mm, pgd, addr);
    pmd = pmd_alloc(mm, pud, addr);
    if(pud == NULL) {
        printk(KERN_WARNING "nvmap: empty pud from addr: %lx\n", addr);
    }

    smp_wmb();
    spin_lock(&mm->page_table_lock);

    pte = rdfs_get_pte(rdfs_get_pmd(ppud));
    set_pmd(pmd, __pmd(rdfs_pa(pte) | _PAGE_TABLE));

    spin_unlock(&mm->page_table_lock);

    sync_global_pgds(addr, end,0);

    return 0;
}


int nvmap_file(struct super_block* sb, unsigned long addr, pud_t *ppud, struct mm_struct *mm)
{
	rdfs_trace();
    unsigned long end = addr + MAX_FILE_SIZE - 1;
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;

    if (!addr || ppud == NULL) {
        printk(KERN_WARNING "nvmap: null pointer error.\n");
        return -EINVAL;
    }

    pgd = pgd_offset(mm, addr);
    do {
        pud = pud_alloc(mm, pgd, addr);
        if(pud == NULL){
            printk(KERN_WARNING "nvmap: empty pud from addr: 0x%lx\n", addr);
            return -ENOMEM;
        }
        
        smp_wmb();
        spin_lock(&mm->page_table_lock);

        pmd = rdfs_get_pmd(ppud);
        rdfs_pud_populate(pud, pmd);

        spin_unlock(&mm->page_table_lock);
        addr = pud_addr_end(addr, end);
        ppud++;
    }while(!pud_none(*ppud));

    sync_global_pgds(addr, end,0);

    return 0;
}


int nvmap(struct super_block* sb, unsigned long addr, pud_t *ppud, struct mm_struct *mm)
{
	rdfs_trace();
    if (!addr || ppud == NULL) {
        printk(KERN_WARNING "nvmap: null pointer error.\n");
        return -1;
    } 

    if (addr >= RDFS_START && 
        addr + MAX_DIR_SIZE - 1 < RDFS_START + DIR_AREA_SIZE) /* map a directory file */
    {
        return nvmap_dir(sb, addr, ppud, mm);
    }
    else if (addr >= RDFS_START + DIR_AREA_SIZE &&
                addr < RDFS_END) /* map a regular file */
    { 
        return nvmap_file(sb, addr, ppud, mm);
    }
    else /* fuck, unknown addr */
    {
        printk(KERN_WARNING "nvmap: unknow addr\n");
        printk(KERN_WARNING "dir: [%lx - %lx]\n", RDFS_START, RDFS_START + DIR_AREA_SIZE - 1);
        printk(KERN_WARNING "file:[%lx - %lx]\n", RDFS_START + DIR_AREA_SIZE, RDFS_END);
        return -1;
    }
}

int unnvmap_file(unsigned long addr, pud_t *ppud, struct mm_struct *mm)
{
	rdfs_trace();
    unsigned long end = addr + MAX_FILE_SIZE - 1;
    pgd_t *pgd;
    pud_t *pud;

    if(ppud == NULL)
	return 0;

    if(!addr) {
        printk(KERN_WARNING "unnvmap: null pointer error.\n");
        return -1;
    }

    pgd = pgd_offset(mm, addr);
    if(unlikely(pgd == 0))
    {
	    printk(KERN_WARNING "unnvmap: empty pgd form addr: 0x%lx\n", addr);
	    return -1;
    }
    while(!pud_none(*ppud))
    {
        pud = pud_offset(pgd, addr);
        if(pud == NULL) {
            printk(KERN_WARNING "unnvmap: empty pud from addr: 0x%lx\n", addr);
            return -1;
        }
        
        pud_clear(pud);
        
        addr = pud_addr_end(addr, end);
        ppud++;
    }
    flush_cache_all();

    return 0;
}

int unnvmap_dir(unsigned long addr, struct mm_struct *mm)
{
	rdfs_trace();
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;

    pgd = pgd_offset(mm, addr);
    if (!pgd) {
        printk(KERN_WARNING "unnvmap_dir: empty pgd entry from addr 0x%lx\n", (unsigned long)addr);
        return -1;
    }

    pud = pud_offset(pgd, addr);
    if (!pud) {
        printk(KERN_WARNING "unnvmap_dir: empty pud entry from addr 0x%lx\n", (unsigned long)addr);
        return -1;
    }

    pmd = pmd_offset(pud, addr);
    if (!pmd) {
        printk(KERN_WARNING "unnvmap_dir: empty pmd entry from addr 0x%lx\n", (unsigned long)addr);
        return -1;
    }
    pmd_clear(pmd);
    flush_cache_all();

    return 0;
}
int unnvmap(unsigned long addr, pud_t *ppud, struct mm_struct* mm)
{
	rdfs_trace();
    if(ppud == NULL)
	return 0;

    if (!addr || ppud == NULL) {
        printk(KERN_WARNING "unnvmap: null pointer error.\n");
        return -1;
    } 

    if (likely(addr >= RDFS_START && addr + MAX_DIR_SIZE - 1 < RDFS_START + DIR_AREA_SIZE)) 
    {
        return unnvmap_dir(addr, mm);
    }
    else if (likely(addr >= RDFS_START + DIR_AREA_SIZE && addr < RDFS_END)) 
    { 
        return unnvmap_file(addr, ppud, mm);
    }
    else 
    {
        printk(KERN_WARNING "unnvmap: unknow addr\n");
        return -1;
    }

}
int  nvmalloc_init(void)
{
	rdfs_trace();
    struct nvm_struct *area; 
    struct nvm_struct *p;
    unsigned long addr = RDFS_START;
    printk(KERN_INFO "nvmalloc: rdfs virtual address space initialized\n");

    area = kzalloc_node(sizeof(*area), GFP_KERNEL | GFP_RECLAIM_MASK,NUMA_NO_NODE);

    area->vaddr = (void*)addr;
    area->size = MAX_DIR_SIZE;
    area->next = NULL;
    dir_free_list = p = area;
    dir_free_cnt++;
    addr = addr + MAX_DIR_SIZE;

    while(addr + MAX_DIR_SIZE - 1 < RDFS_START + DIR_AREA_SIZE)
    {
        area = kzalloc_node(sizeof(*area), GFP_KERNEL | GFP_RECLAIM_MASK, NUMA_NO_NODE);
        area->vaddr = (void*)addr;
        area->size = MAX_DIR_SIZE;
        area->next = NULL;
        p->next = area;
        p = area;
        addr = addr + MAX_DIR_SIZE;
        dir_free_cnt++;
    }

    printk(KERN_INFO "Directory space area[total = %d, size = 0x%lx]:\n", dir_free_cnt, MAX_DIR_SIZE);
    printk(KERN_INFO "[%lx %lx]\n", (unsigned long)(dir_free_list->vaddr),(unsigned long)(area->vaddr) + MAX_DIR_SIZE);


    addr = RDFS_START + DIR_AREA_SIZE;
    area = kzalloc_node(sizeof(*area), GFP_KERNEL | GFP_RECLAIM_MASK, NUMA_NO_NODE);
    area->vaddr =(void*)addr;
    area->size = MAX_FILE_SIZE;
    area->next = NULL;
    reg_free_list = p = area;
    reg_free_cnt++; 
    addr = addr +  MAX_FILE_SIZE;
    while(addr + MAX_FILE_SIZE - 1 < RDFS_END)
    {
        area = kzalloc_node(sizeof(*area), GFP_KERNEL | GFP_RECLAIM_MASK, NUMA_NO_NODE);
        area->vaddr = (void*)addr;
        area->size = MAX_FILE_SIZE;
        area->next = NULL;
        p->next = area;
        p = area;
        addr =  addr + MAX_FILE_SIZE;
        reg_free_cnt++;
    }
    printk(KERN_INFO "File space area[total = %d, size = 0x%lx]:\n", reg_free_cnt, MAX_FILE_SIZE);
    printk(KERN_INFO "[%lx %lx]\n", (unsigned long)(reg_free_list->vaddr), (unsigned long)(area->vaddr) + MAX_FILE_SIZE);
    
    return 0;
}
