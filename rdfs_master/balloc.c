/* @author page
 * @date 2017/12/3
 * rdfs/balloc.c
 */


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
#include "debug.h"
#include "pgtable.h"

#include "balloc.h"
#define TRYTIME (1000000)


static struct file_system_type **rdfs_find_filesystem(const char *name, unsigned len)
{
//	rdfs_trace();
	extern struct file_system_type *file_systems;
	struct file_system_type **p;
	for (p=&file_systems; *p; p=&(*p)->next)
		if (strlen((*p)->name) == len &&
		    strncmp((*p)->name, name, len) == 0)
			break;
	return p;
}

sector_t rdfs_get_bdev_size(struct block_device* bdev)
{
//	rdfs_trace();
	return bdev->bd_part->nr_sects>>3;
}



size_t rdfs_init_bdev(struct rdfs_sb_info *sbi,char* path,unsigned long * bdev_address)
{
	//rdfs_trace();
	struct file_system_type **p;
	struct bdev_des* bdev_p;
	fmode_t mode;
	mode = FMODE_READ | FMODE_EXCL | FMODE_WRITE;
	bdev_p = (struct bdev_des*)(*bdev_address);
	
	p = rdfs_find_filesystem("rdfsfs", 6);
	bdev_p->bdev = blkdev_get_by_path(path, mode, *p);
	if (IS_ERR(bdev_p->bdev))
	{
		printk("err:can not get the bdev:%s\n",path);
		return 0;
	}

	++sbi->bdev_count;

	spin_lock_init(&bdev_p->spinlock);
	bdev_p->start = 1;
	bdev_p->end = rdfs_get_bdev_size(bdev_p->bdev);
	*bdev_address = (unsigned long)(bdev_p+1);

	return 1;
}
struct numa_des* get_cpu_numa(void)
{
	//rdfs_trace();
	struct rdfs_super_block * nsb = rdfs_get_super(RDFS_SUPER_BLOCK_ADDRESS);
	if (nsb->s_numa_flag && ((1<<smp_processor_id())&NUMA_MASK))
		return (struct numa_des*)nsb->s_numa_des[1];
	else
		return (struct numa_des*)nsb->s_numa_des[0];
}
int rdfs_new_block(phys_addr_t *physaddr,int num,struct rdfs_inode* ni)
{
//	rdfs_trace();
	unsigned long *next;
	int errval = 0, i;
	unsigned try_time = 0;
	unsigned long irqflags;
	struct numa_des* numa_des_p = get_cpu_numa();

	if (ni!=NULL)
	{
		numa_des_p = rdfs_get_numa_by_inode(ni);
	}

	if(numa_des_p->free_page_count < num)
	{	
		printk("%s -- not enough memory \n",__FUNCTION__);
		return -1;
	}
	spin_lock(&numa_des_p->free_list_lock);
	if(unlikely(numa_des_p->free_page_count<num))
	{
		spin_unlock(&numa_des_p->free_list_lock);
		printk("%s free block not enough!!!!\n",__FUNCTION__);
		return -1;
	}
	*physaddr = numa_des_p->free_page_phy;
	
	next = (unsigned long*)rdfs_va(numa_des_p->free_page_phy);

	for (i = 0; i < num; ++i)
	{
		next = (unsigned long *)rdfs_va(*next);
	}
	numa_des_p->free_page_count -= num;
	numa_des_p->free_page_phy = rdfs_pa(next);
	spin_unlock(&numa_des_p->free_list_lock);

fail:
	return errval;
}
void* rdfs_get_page(void)
{
//	rdfs_trace();
	phys_addr_t physaddr = 0;
	void* virt_addr;
	rdfs_new_block(&physaddr, 1, NULL);
	if(physaddr != 0)
	{
		virt_addr = rdfs_va(physaddr);
	}
	return virt_addr;
}
void bdev_insert_a_page(unsigned long address)
{
//	rdfs_trace();
	extern struct mm_struct init_mm;
	pgd_t* pgd=pgd_offset(&init_mm,address);
	pud_t* pud=pud_alloc(&init_mm,pgd,address);
	pmd_t* pmd=rdfs_pmd_alloc(RDFS_SUPER_BLOCK_ADDRESS,pud,address);
	pte_t* pte=rdfs_pte_alloc(RDFS_SUPER_BLOCK_ADDRESS,pmd,address);
	if(pte_val(*pte)==0)
	{
		rdfs_set_pte(pte,rdfs_pa(rdfs_get_page()));
	}
}

void print_pgtable(struct mm_struct* mm,unsigned long address)
{
//	rdfs_trace();
	pgd_t* pgd;
	pud_t* pud;
	pmd_t* pmd;
	pte_t* pte;
	if(mm==NULL)
	{
		printk("mm none\n");
	}

	pgd=pgd_offset(mm,address);
	if(pgd_val(*pgd)==0)
	{
		printk("pgd none\n");
		return;
	}

	pud=pud_offset(pgd,address);
	if(pud_val(*pud)==0)
	{
		printk("pud none\n");
		return;
	}	

	pmd=rdfs_pmd_offset(pud,address);
	if(pmd_val(*pmd)==0)
	{
		printk("pmd none\n");
		return;
	}

	pte=rdfs_pte_offset(pmd,address);
	if(pte_val(*pte)==0)
	{
		printk("pte none\n");
		return;
	}
	printk("mm:%lx cr3:%lx\n",mm,read_cr3());
	printk("pgd:%lx val:%lx \n \
		pud:%lx val:%lx \n \
		pmd:%lx val:%lx \n \
		pte:%lx val:%lx\n",pgd,pgd_val(*pgd),pud,pud_val(*pud),pmd,pmd_val(*pmd),pte,pte_val(*pte));
	pgd = __va(read_cr3());
	pgd += pgd_index(address);
	printk("cr3 pgd val:%lx\n",pgd_val(*pgd));
	if(pgd_val(*pgd)==0)
	{
		printk("pgd none\n");
		return;
	}	
	
	pud=pud_offset(pgd,address);
	if(pud_val(*pud)==0)
	{
		printk("pud none\n");
		return;
	}	

	pmd=rdfs_pmd_offset(pud,address);
	if(pmd_val(*pmd)==0)
	{
		printk("pmd none\n");
		return;
	}

	pte=rdfs_pte_offset(pmd,address);
	if(pte_val(*pte)==0)
	{
		printk("pte none\n");
		return;
	}
	printk("pgd:%lx val:%lx \n \
		pud:%lx val:%lx \n \
		pmd:%lx val:%lx \n \
		pte:%lx val:%lx\n",pgd,pgd_val(*pgd),pud,pud_val(*pud),pmd,pmd_val(*pmd),pte,pte_val(*pte));
		
}
EXPORT_SYMBOL(print_pgtable);

void unexpected_do_fault(unsigned long address)
{
	//rdfs_trace();
	extern struct mm_struct init_mm;
	pgd_t* pgd_k;
	pgd_t* pgd;
	int index = pgd_index(address);
	pgd_k = init_mm.pgd + index;
	pgd   = __va(read_cr3());
	pgd  += index;
	*pgd  = *pgd_k;
}

static void __rdfs_init_bdev_pool(struct bdev_des* bdev_des,unsigned long address,unsigned long capacity)
{
	int i=0;
	extern struct mm_struct init_mm;
	unsigned long addr = address;
	for (i=0;i<=CPU_NR;++i)
	{
		spin_lock_init(&(bdev_des->block_pool[i].spinlock));
		bdev_des->block_pool[i].post=0;
		bdev_des->block_pool[i].capacity=capacity;
		bdev_des->block_pool[i].address=(unsigned long*)addr;
		address=(unsigned long)bdev_des->block_pool[i].address;
		while(address<(unsigned long)(bdev_des->block_pool[i].address+bdev_des->block_pool[i].capacity))
		{
			bdev_insert_a_page(address);
			print_pgtable(&init_mm,address);
			address+=PAGE_SIZE;
		}
		addr+=MAX_CAPACITY*sizeof(sector_t);
	}
    flush_cache_all();
}

void rdfs_init_bdev_pool(struct super_block* sb)
{
	int i;
//	rdfs_trace();
	extern struct mm_struct init_mm;
	struct rdfs_sb_info* sbi = RDFS_SB(sb);
	struct bdev_des* bdev_des = (struct bdev_des*)((unsigned long) sbi->virt_addr + RDFS_SB_SPACE+ADDRESS_TO_INO_SPACE);
	unsigned long address = BDEV_VIRT_ADDR;
	pgd_t* pgd;
	pud_t* pud;
	for (i=0;i<sbi->bdev_count;++i)
	{
		pgd=pgd_offset(&init_mm,address);
		pud=pud_alloc(&init_mm,pgd,address);

		bdev_des[i].pgtable=rdfs_pa(rdfs_get_zeroed_page());
		bdev_des[i].free_pmd_page=rdfs_pa(rdfs_get_zeroed_page());
		set_pud(pud, __pud(bdev_des[i].pgtable | _PAGE_TABLE));
		__rdfs_init_bdev_pool(&bdev_des[i],address,DEFAYLT_CAPACITY);
		address+=BDEV_VIRT_AERA;
	}
}

unsigned long rdfs_get_bdev(unsigned long sbi_addr, void **data)
{
	char *options = (char *) *data;
	char *end;
	char org_end;
//	rdfs_trace();
	struct rdfs_sb_info *sbi = (struct rdfs_sb_info*)sbi_addr;
	unsigned long bdev_address = (unsigned long)sbi->virt_addr+RDFS_SB_SPACE+ADDRESS_TO_INO_SPACE;
	sbi->bdev_count = 0;
	if (!options || strncmp(options, "bdev=", 5) != 0 )
		return bdev_address;
	options=options+5;
	while(strncmp(options, "/dev", 4) == 0){
		end = strchr(options, ',') ?: options + strlen(options);
		org_end = *end;
		*end = '\0';
		rdfs_init_bdev(sbi,options,&bdev_address);
		*end = org_end;
		options = end;
		if (*options == ',')
			options++;
	}
  	*data = (void *) options;

  	return bdev_address;
}

void rdfs_init_free_inode_list_offset(struct numa_des* numa_des_p)
{
	//rdfs_trace();
    void *start_addr = rdfs_va(numa_des_p->free_inode_phy);
    unsigned int inode_count =  numa_des_p->free_inode_count;
    struct rdfs_inode *temp = NULL;
    void* next = start_addr + RDFS_INODE_SIZE;

    while(inode_count > 0) {
        temp = (struct rdfs_inode *)start_addr;
        temp->i_pg_addr = rdfs_pa(next);
        start_addr += RDFS_INODE_SIZE;
        next += RDFS_INODE_SIZE;
        inode_count--;
    }
}

void rdfs_init_free_block_list_offset(struct numa_des* numa_des_p)
{
	//rdfs_trace();
    unsigned long start_addr = numa_des_p->free_page_phy;
    unsigned long page_count = numa_des_p->free_page_count; 
    unsigned long *temp = (unsigned long*)rdfs_va(start_addr);
    unsigned long next = start_addr + PAGE_SIZE;
    while(page_count > 0) {
        *temp = next;
        next += PAGE_SIZE;
        temp  = (unsigned long*)((unsigned long)temp + PAGE_SIZE);
        --page_count;
    }
}

int rdfs_init_numa(unsigned long start , unsigned long end , unsigned long percentage)
{
	//rdfs_trace();
	struct numa_des * numa_des_p = (struct numa_des *) (rdfs_va(start));
	if (end<start)
		return -1;
	memset(rdfs_va(start),0x00,sizeof(struct numa_des)+RDFS_INODE_SIZE);
	
	spin_lock_init(&numa_des_p->inode_lock);
	spin_lock_init(&numa_des_p->free_list_lock);
	rwlock_init(&numa_des_p->lru_rwlock);
	numa_des_p->free_inode_phy = start + sizeof(struct numa_des) + RDFS_INODE_SIZE;
	numa_des_p->free_inode_count   = (end - start)*percentage / 100 / RDFS_INODE_SIZE -1;
	if (numa_des_p->free_inode_count <= 0)
		return -1;
	numa_des_p->free_page_phy  = numa_des_p->free_inode_phy + numa_des_p->free_inode_count*RDFS_INODE_SIZE;
	numa_des_p->free_page_phy  = (numa_des_p->free_page_phy + PAGE_SIZE - 1)&~(PAGE_SIZE-1);
	numa_des_p->free_page_count    = (end - numa_des_p->free_page_phy)>>PAGE_SHIFT;

	rdfs_init_free_inode_list_offset(numa_des_p);
	rdfs_init_free_block_list_offset(numa_des_p);

	return 0;
}

static int next_numa_id = 0;

struct numa_des* get_a_numa(void)
{
//	rdfs_trace();
	struct rdfs_super_block * nsb = rdfs_get_super(RDFS_SUPER_BLOCK_ADDRESS);
	if (nsb->s_numa_flag)
	{
		++next_numa_id;
		return (struct numa_des*)nsb->s_numa_des[next_numa_id&0x1];
	}
	else
		return (struct numa_des*)nsb->s_numa_des[0];
}

struct numa_des* get_rdfs_inode_numa(struct rdfs_inode* inode)
{
//	rdfs_trace();
	struct rdfs_super_block * nsb = rdfs_get_super(RDFS_SUPER_BLOCK_ADDRESS);
	if (nsb->s_numa_flag && (rdfs_pa(inode)>rdfs_NUMA_LINE))
		return (struct numa_des*)nsb->s_numa_des[1];
	else
		return (struct numa_des*)nsb->s_numa_des[0];
}




void* rdfs_get_zeroed_page()
{
//	rdfs_trace();
	phys_addr_t physaddr = 0;
	void* virt_addr = NULL;
	rdfs_new_block(&physaddr, 1 ,NULL);
	if (physaddr != 0)
	{
		virt_addr = rdfs_va(physaddr);
   		memset(virt_addr , 0x00, PAGE_SIZE);
	}
	printk("%s phy_addr:%lx virt_addr:%lx\n",__FUNCTION__,physaddr,virt_addr);
	return virt_addr;
}


void rdfs_free_block(struct super_block *sb, phys_addr_t phys )
{
//	rdfs_trace();
	struct rdfs_super_block *nsb;
	int numa_id;
	struct numa_des* numa_des_p;
	unsigned long irqflags;
	nsb = rdfs_get_super(sb);
	if (nsb->s_numa_flag && phys > rdfs_NUMA_LINE)
		numa_id = 1;
	else
		numa_id = 0;

	numa_des_p = (struct numa_des*)nsb->s_numa_des[numa_id];
	spin_lock(&numa_des_p->free_list_lock);
	
	*(unsigned long*)rdfs_va(phys) = numa_des_p->free_page_phy;
	numa_des_p->free_page_phy = phys;
	++numa_des_p->free_page_count;
	spin_unlock(&numa_des_p->free_list_lock);
 
}


unsigned long rdfs_count_free_blocks(struct super_block *sb)
{
	//rdfs_trace();
	struct rdfs_super_block *nsb = rdfs_get_super(sb);
	unsigned long count = ((struct numa_des*)(nsb->s_numa_des[0]))->free_page_count;
	if (nsb->s_numa_des[1]!=0)
		count += ((struct numa_des*)(nsb->s_numa_des[1]))->free_page_count;
	return count;
}

struct bdev_des* rdfs_alloc_bdev(unsigned long * bdev_id)
{
	//rdfs_trace();
	struct rdfs_sb_info* sbi;
	struct bdev_des* bdev_des;

	sbi=RDFS_SB(RDFS_SUPER_BLOCK_ADDRESS);
	*bdev_id = atomic_inc_return(&sbi->atomic_bdev_post)%sbi->bdev_count;
	bdev_des = (struct bdev_des*)(sbi->virt_addr + BDEV_DES_OFFSET+(*bdev_id)*sizeof(struct bdev_des));
	return bdev_des;
}

sector_t __rdfs_alloc_a_disk_block(struct bdev_des* bdev_des)
{
	//rdfs_trace();
	int cpu_id = smp_processor_id();
	unsigned long * virt_addr = NULL;
	int count = 0 ;
	int start = 0 ;
	int i=0;
	sector_t sn = -1;
	unsigned long irqflags;

	extern struct mm_struct init_mm;	

	struct block_pool_per_cpu* block_pool = &bdev_des->block_pool[cpu_id];
	struct block_pool_per_cpu* reserved_pool = &bdev_des->block_pool[CPU_NR];
	virt_addr = block_pool->address;
	spin_lock(&block_pool->spinlock);
	if (block_pool->post == 0 )
	{
		spin_lock(&bdev_des->spinlock);
		if (reserved_pool->post == 0)
		{
			if (bdev_des->start == bdev_des->end)
			{
				printk("no space on device \n");
				spin_unlock(&bdev_des->spinlock);
				spin_unlock(&block_pool->spinlock);
				return -1;
			}
			start = bdev_des->start;
			if (bdev_des->start + COUNT_PER_ALLOC -1<= bdev_des->end)
			{
				count = COUNT_PER_ALLOC ;
				bdev_des->start = bdev_des->start + COUNT_PER_ALLOC;
			}
			else
			{
				count = bdev_des->end - bdev_des->start +1;
				bdev_des->start = bdev_des->end;
			}
			for (i=0;i<count;++i,++start)
			{
				virt_addr[i] = start ;
			}
			block_pool->post = count;  
		}
		else
		{
			if (reserved_pool->post>COUNT_PER_ALLOC)
			{
				count = COUNT_PER_ALLOC;
			}
			else
			{
				count= reserved_pool->post;
			}
			reserved_pool->post-= count;
			memcpy(virt_addr,reserved_pool->address+sizeof(sector_t)*count,sizeof(sector_t)*count);	
			block_pool->post += count;
		}
		spin_unlock(&bdev_des->spinlock);
	}	
	sn = virt_addr[--(block_pool->post)];

	spin_unlock(&block_pool->spinlock);

	return sn;

}


sector_t rdfs_alloc_a_disk_block(struct bdev_des* bdev_des)
{
//	rdfs_trace();
	return __rdfs_alloc_a_disk_block(bdev_des);
}

static inline void free_into_pool(struct block_pool_per_cpu* block_pool,sector_t sn)
{
//	rdfs_trace();
	unsigned long* virt_addr = block_pool->address;
	unsigned long* capacity_addr;
	if(block_pool->post>=block_pool->capacity)
	{
		capacity_addr = virt_addr + block_pool->capacity;
		bdev_insert_a_page((unsigned long)capacity_addr);
		block_pool->capacity+=PAGE_SIZE/sizeof(sector_t);
	}
	virt_addr[block_pool->post++]=sn;
}

static unsigned long __rdfs_free_a_disk_block(struct bdev_des* bdev_des,sector_t sn)
{
//	rdfs_trace();
	int cpu_id = smp_processor_id();
	struct block_pool_per_cpu* block_pool = &bdev_des->block_pool[cpu_id];
	struct block_pool_per_cpu* reserved_pool = &bdev_des->block_pool[CPU_NR];
	
	unsigned long irqflags;
	spin_lock(&block_pool->spinlock);

	if (block_pool->post == block_pool->capacity)
	{
		spin_lock(&reserved_pool->spinlock);
		free_into_pool(reserved_pool,sn);
		spin_unlock(&reserved_pool->spinlock);
	}
	else
	{
		free_into_pool(block_pool,sn);
	}
	spin_unlock(&block_pool->spinlock);
	
	return 1;	
}

unsigned long rdfs_free_a_disk_block(struct bdev_des* bdev_des, sector_t sn)
{
	//rdfs_trace();
	return __rdfs_free_a_disk_block(bdev_des,sn);
}

void rdfs_pre_free_init(struct pre_free_struct* pre_free_struct)
{
	rdfs_trace();
	pre_free_struct->pre_free_struct[0].free_start = 0;
	pre_free_struct->pre_free_struct[0].free_end = 0;
	pre_free_struct->pre_free_struct[0].free_count = 0;

	pre_free_struct->pre_free_struct[1].free_start = 0;
	pre_free_struct->pre_free_struct[1].free_end = 0;
	pre_free_struct->pre_free_struct[1].free_count = 0;
}

void rdfs_pre_free_block(struct pre_free_struct* pre_free_struct,phys_addr_t phys)
{
//	rdfs_trace();
	int numa_id = rdfs_get_phys_numa_id(phys);
	struct __pre_free_struct* __pre_free_struct = &(pre_free_struct->pre_free_struct[numa_id]);

	if(__pre_free_struct->free_count==0)
	{
		__pre_free_struct->free_start = phys;
		__pre_free_struct->free_end   = phys;
	}
	else
	{
		*(phys_addr_t*)rdfs_va(phys) = __pre_free_struct->free_start;
		__pre_free_struct->free_start = phys;
	}
	++__pre_free_struct->free_count;
}

void rdfs_pre_free_list(struct super_block* sb , struct pre_free_struct* pre_free_struct)
{
//	rdfs_trace();
	struct rdfs_super_block * nsb;
	struct numa_des* numa_des_p;
	struct __pre_free_struct * __pre_free_struct;
	unsigned long irqflags;
	int i;
	nsb = rdfs_get_super(sb);
	for (i=0;i<2;++i)
	{
		numa_des_p = (struct numa_des*)nsb->s_numa_des[i];
		__pre_free_struct = &pre_free_struct->pre_free_struct[i];
		if (__pre_free_struct->free_count!=0)
		{
			spin_lock(&numa_des_p->free_list_lock);
			
			*(unsigned long*)rdfs_va(__pre_free_struct->free_end) = numa_des_p->free_page_phy;
			numa_des_p->free_page_phy = __pre_free_struct->free_start;
			numa_des_p->free_page_count+=__pre_free_struct->free_count;
			spin_unlock(&numa_des_p->free_list_lock);

		}
	}
}
