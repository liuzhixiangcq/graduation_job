/* @author page
 * @date 2017/12/3
 * rdfs/pgtable.c
 */


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
#include "nvmalloc.h"
#include "debug.h"
#include "balloc.h"
#include "memory.h"
#include "rdfs_config.h"
#include "inode.h"
#include "network.h"
static inline pud_t *rdfs_pud_alloc_one(void)
{
	//rdfs_trace();
    return (pud_t*)rdfs_get_zeroed_page();
}

static inline pmd_t *rdfs_pmd_alloc_one(void)
{
	//rdfs_trace();
    return (pmd_t*)rdfs_get_zeroed_page();
}

static inline pte_t *rdfs_pte_alloc_one(void)
{
	//rdfs_trace();
    return (pte_t*)rdfs_get_zeroed_page();
}


static inline void rdfs_pud_free(struct super_block *sb, pud_t *pud)
{
	//rdfs_trace();
    rdfs_free_block(sb, rdfs_pa(pud));
}

static inline void rdfs_pmd_free(struct super_block *sb, pmd_t *pmd)
{
	//rdfs_trace();
    rdfs_free_block(sb, rdfs_pa(pmd));
}

static inline void rdfs_pte_free(struct super_block *sb, pte_t *pte)
{
	//rdfs_trace();
    rdfs_free_block(sb, rdfs_pa(pte));
}

int __rdfs_pmd_alloc(struct super_block *sb, pud_t *pud)
{
//	rdfs_trace();
    pmd_t *new = rdfs_pmd_alloc_one();
    if (!new)
        {
            printk("error :%s \n",__FUNCTION__);
            return -ENOMEM;
        }
    smp_wmb();

    if(pud_present(*pud)) 
        rdfs_pmd_free(sb, new);
    else
        rdfs_set_pud(pud, new);

    return 0;
}

int __rdfs_pte_alloc(struct super_block *sb, pmd_t *pmd)
{
//	rdfs_trace();
    pte_t *new = rdfs_pte_alloc_one();
    if (!new)
        {
            printk("error :%s \n",__FUNCTION__);
            return -ENOMEM;
        }
    smp_wmb();

    if(pmd_present(*pmd))
        rdfs_pte_free(sb, new);
    else
        rdfs_set_pmd(pmd, new);

    return 0;
}
pud_t *rdfs_pud_alloc(struct super_block *sb, unsigned long ino, unsigned long offset)
{
//	rdfs_trace();
    int i = 0;
    int nr = offset >> PUD_SHIFT;
    pud_t *pud = rdfs_get_pud(sb, ino);
    while(i++ < nr) pud++;

    return pud;
}


pmd_t *rdfs_pmd_alloc(struct super_block *sb, pud_t *pud, unsigned long addr)
{
//	rdfs_trace();
    if (addr >= RDFS_START && 
		    addr + MAX_DIR_SIZE - 1 < RDFS_START + DIR_AREA_SIZE )
    {
    	return rdfs_get_pmd(pud);	    
    }
    else {
    	return (unlikely(pud_none(*pud)) && __rdfs_pmd_alloc(sb, pud))?
        	NULL: rdfs_pmd_offset(pud, addr);
    }
}

pte_t *rdfs_pte_alloc(struct super_block *sb, pmd_t *pmd, unsigned long addr)
{
//	rdfs_trace();
    return (unlikely(pmd_none(*pmd)) && __rdfs_pte_alloc(sb, pmd))?
        NULL: rdfs_pte_offset(pmd, addr);
}

int rdfs_init_pg_table(struct super_block * sb, u64 ino)
{
//    rdfs_trace();
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct rdfs_inode *ni = get_rdfs_inode(sb, ino);
//    printk("%s ni:%lx\n",__FUNCTION__,ni);
    pud = rdfs_pud_alloc_one();
    pmd = rdfs_pmd_alloc_one();
    pte = rdfs_pte_alloc_one();
    
    if (pud == NULL || pmd == NULL || pte == NULL) {
        printk("%s init file page table failed\n",__FUNCTION__);
        //rdfs_error(sb, __FUNCTION__, "init file page table failed!\n");
        return -PGFAILD;
    }


    rdfs_set_pmd(pmd, pte);
    rdfs_set_pud(pud, pmd);

    ni->i_pg_addr = rdfs_pa(pud);
    printk("%s pg_addr:%lx pud:%x->pmd:%lx,pmd:%lx->pte:%lx pte:%lx->pte val:%lx\n",__FUNCTION__,ni->i_pg_addr,pud,*pud,pmd,*pmd,pte,*pte);
    return 0;
}

int rdfs_insert_page(struct super_block *sb, struct inode *vfs_inode, phys_addr_t phys)
{
    rdfs_trace();
    unsigned long ino = 0;
    unsigned long addr = 0;
    unsigned long offset = 0;
    unsigned long new_addr = 0;
    unsigned long blocks = 0;

    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte; 

    ino = vfs_inode->i_ino;
    addr = (unsigned long)(RDFS_I(vfs_inode))->i_virt_addr;
   // printk("################: i_virt_addr:%lx pte_info:%lx pte_addr:%lx pte_addr:%lx \n",addr,*(unsigned long *)(rdfs_va(phys)),phys,phys| __PAGE_KERNEL);
    blocks = vfs_inode->i_blocks;
    offset = ((blocks - 1) << PAGE_SHIFT);
    new_addr = addr + offset;
  //  printk("blocks:%lx new_addr:%lx\n",blocks,new_addr);
    pud = rdfs_pud_alloc(sb, ino, offset);

    if (unlikely(!pud)){
        printk(KERN_INFO "rdfs_empty pud!!!\n");
        return -1;
    }

    if(unlikely(pud_none(*pud))) {  /* insert to kernel page table */
        pmd = rdfs_pmd_alloc(sb, pud, new_addr);    
        nvmap_pmd(sb, new_addr, rdfs_get_pmd(pud), current->mm);
    }else
        pmd = rdfs_pmd_alloc(sb, pud, new_addr);

    if (unlikely(!pmd)) {
        printk(KERN_INFO "empty pmd!!!\n");
        return -1;
    }
    pte = rdfs_pte_alloc(sb, pmd, new_addr);
    if (unlikely(!pte)) {
        printk(KERN_INFO "empty pte!!!\n");
        return -1;  
    }
	rdfs_set_pte(pte, phys);
    return 0;
}

void rdfs_rm_pte_range(struct super_block *sb, struct rdfs_inode* nv_i, pmd_t *pmd)
{
//	rdfs_trace();
    pte_t *pte, *p;
    int cnt = 0;
    unsigned long phys;
    unsigned long sn;
    unsigned long bdev_id;
    p = pte = rdfs_get_pte(pmd);

   while(!pte_none(*pte) && cnt < PTRS_PER_PTE)
   {
        phys = rdfs_get_phy_from_pte(pte);
		
        if (pte_is_sn(pte))
		{
			sn=get_sn_from_pte(pte);
			bdev_id = get_bdev_id_from_pte(pte);
			rdfs_free_a_disk_block(get_bdev_des_by_id(bdev_id),sn);
		}
		else
        {
        	rdfs_free_block(sb, phys);
        }
        pte++;
        cnt++;
    }

    rdfs_pte_free(sb, p);
}


void rdfs_rm_pmd_range(struct super_block *sb,struct rdfs_inode* nv_i, pud_t *pud)
{
//	rdfs_trace();
    pmd_t *pmd, *p;
    int cnt = 0;

    p = pmd = rdfs_get_pmd(pud);


    while(!pmd_none(*pmd) && cnt < PTRS_PER_PMD)
    {
        rdfs_rm_pte_range(sb, nv_i, pmd);
        pmd++;
        cnt++;
    }

    rdfs_pmd_free(sb, p);
}



void rdfs_rm_pg_table(struct super_block *sb, u64 ino)
{
//	rdfs_trace();
    pud_t *pud, *p;
    struct rdfs_inode *ni;
    int cnt = 0;

    ni = get_rdfs_inode(sb, ino);
    p = pud = rdfs_get_pud(sb, ino);

    while(!pud_none(*pud) && cnt < PTRS_PER_PUD)
    {
        rdfs_rm_pmd_range(sb, ni,pud);
        pud++;
        cnt++;
    }

    rdfs_pud_free(sb, p);

    ni->i_pg_addr = 0;
}

int rdfs_establish_mapping(struct inode *inode)
{
	rdfs_trace();
	struct rdfs_inode_info *ni_info;
	struct super_block *sb;
	int errval = 0;
	pud_t *pud;
	int mode = 0;
	unsigned long ino;
	unsigned long vaddr;
	struct mm_struct *mm;
	struct inode** p_address;

	ni_info = RDFS_I(inode);
	sb = inode->i_sb;
	ino = inode->i_ino;
	pud = rdfs_get_pud(sb, ino);

	mm = &init_mm;
    ++ni_info->mapping_count;

	vaddr = (unsigned long) ni_info->i_virt_addr;

	if(!vaddr){
		if((S_ISDIR(inode->i_mode)) || (S_ISLNK(inode->i_mode))) 
		{
		//	printk("%s i_mode is dir\n",__FUNCTION__);
			mode = 1;
			ni_info->i_virt_addr = nvmalloc(mode);
		}
		else if(S_ISREG(inode->i_mode))
		{
         //   printk("%s i_mode is reg file\n",__FUNCTION__);
			mode = 0;
			ni_info->i_virt_addr = nvmalloc(mode);				
		    p_address=rdfs_address_to_ino(sb,(unsigned long)ni_info->i_virt_addr);
		    *p_address=inode;
		}
	}else
	{
		printk("this is a multiple process");
	}
	vaddr = (unsigned long) ni_info->i_virt_addr;

	if ((vaddr >= RDFS_START && vaddr + MAX_DIR_SIZE - 1 < RDFS_START + DIR_AREA_SIZE) || 
		(vaddr >= RDFS_START + DIR_AREA_SIZE && vaddr < RDFS_END))
    {
		errval = nvmap(sb, vaddr, pud, mm);
    }
    else
    {
    	printk(KERN_WARNING "nvmap: unknow addr\n");
    	return -1;
    }
	return errval;
}

int rdfs_destroy_mapping(struct inode *inode)
{
	rdfs_trace();
	struct rdfs_inode_info *ni_info;
	struct super_block *sb;
	int errval = 0;
	pud_t *pud;
	unsigned long ino;
	unsigned long vaddr;
	ni_info = RDFS_I(inode);
	vaddr = (unsigned long)ni_info->i_virt_addr;
	sb = inode->i_sb;
	ino = inode->i_ino;
	pud = rdfs_get_pud(sb, ino);

	if(!vaddr)
	{
        printk("error :%s \n",__FUNCTION__);
    	return 0;
    }
	if(--ni_info->mapping_count>0)
	{
        return 0;
    }
	errval = unnvmap(vaddr, pud, &init_mm);
	nvfree(ni_info->i_virt_addr);
	ni_info->i_virt_addr = 0;
	return errval;
}

int rdfs_search_metadata(struct rdfs_inode_info * ri_info,unsigned long offset,unsigned long *s_id,unsigned long *block_id)
{
    rdfs_trace();
    printk("%s i_virt_addr:%lx offset:%lx vaddr:%lx\n",__FUNCTION__,ri_info->i_virt_addr,offset,(unsigned long*)(ri_info->i_virt_addr+offset / 4096 * 8));
    unsigned long pte_value = 0;
    pte_value = *(unsigned long *)(ri_info->i_virt_addr + offset / 4096 * 8);
    printk("*******:pte_value:%lx\n",pte_value);
    *s_id = (pte_value & SLAVE_ID_MASK) >> SLAVE_ID_SHIFT;
    *block_id = (pte_value & BLOCK_ID_MASK) >> BLOCK_ID_SHIFT;
    return 0;
}
int rdfs_client_search_metadata(struct client_request_task*task,int ino,long offset,long length)
{
    rdfs_trace();
    struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
    struct rdfs_inode* ri = get_rdfs_inode(sb,ino);
    struct inode* inode = rdfs_iget(sb,ino);
    struct rdfs_inode_info * ri_info = RDFS_I(inode);
    loff_t i_size=0;
    i_size = i_size_read(inode);
    //rdfs_establish_mapping(inode);
    long pages_exist = 0,pages_to_alloc = 0,pages_needed = 0;
	pages_needed = (offset + length + sb->s_blocksize -1) >> sb->s_blocksize_bits;
	pages_exist = (i_size + sb->s_blocksize - 1) >> sb->s_blocksize_bits;
    pages_to_alloc = pages_needed - pages_exist;
    int retval;
	if(pages_to_alloc > 0)
	{
		retval = rdfs_alloc_blocks(inode,pages_to_alloc,1,ALLOC_PTE);
		if(retval < 0)
		{
			printk("%s --> rdfs_alloc_blocks failed\n",__FUNCTION__);
			return retval;
		}
	}
	unsigned long start = offset,end = offset + length;
	unsigned long i = 0,pte = 0,block_offset = 0;
	unsigned long s_id = 0,block_id = 0;
    unsigned long size = 0;
    int cnt = 0;
    int back_cnt = 0;
	for(i=start;i<end;)
	{
		pte = (i >> RDFS_BLOCK_SHIFT) << RDFS_BLOCK_SHIFT;
		block_offset = i & RDFS_BLOCK_OFFSET_MASK;
		
		size = RDFS_BLOCK_SIZE - block_offset;
		printk("pte:%ld,block_offset:%ld block_size:%ld size:%ld\n",pte,block_offset,RDFS_BLOCK_SIZE,size);
		if(i+size > end)
			size = (unsigned long)((long)end - (long)i);
        retval = rdfs_search_metadata(ri_info,pte,&s_id,&block_id);
        cnt += sprintf(task->message->m_data + cnt,"%ld:%ld:%ld:%ld",s_id,block_id,block_offset,size);
		printk("%s rdfs_seach_metadata:s_id:%lx block_id:%lx block_offset:%lx size:%lx\n",__FUNCTION__,s_id,block_id,block_offset,size);
        i += RDFS_BLOCK_SIZE - block_offset;
        back_cnt ++;
    }
    task->message->nums = back_cnt;
	if(offset + length > i_size)
	{	
		i_size_write(inode, offset + length);
	}
    i_size_write(inode,offset + length);
    return 0;
}
