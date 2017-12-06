#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <asm-generic/bitops/lock.h>
#include <linux/bio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "rdfs.h"
#include "server.h"
#include "debug.h"
#include "balloc.h"
#include "rdfs_swap.h"
extern int is_modify;
extern int posted_req_cnt;
struct private_data
{
	atomic_t bio_cnt;
	struct page * bio_page;
};
extern struct rdfs_context ctx[MAX_CLIENT_NUMS];
void rdfs_bio_endio_in(struct bio* bio,int error)
{
	rdfs_trace();
	struct private_data * data = (struct private_data*)bio->bi_private;
	if (bio->bi_io_vec[0].bv_page!=data->bio_page)
	{
		unlock_page(bio->bi_io_vec[0].bv_page);
	}
	if (atomic_dec_and_test(&(data->bio_cnt)))
	{
		unlock_page(data->bio_page);
	}
}

bool get_next_pte(pud_t** pudp,pmd_t** pmdp,pte_t** ptep)
{
	rdfs_trace();
	++(*ptep);
	if (unlikely((((unsigned long)*ptep)&0xfff)==0))
	{
		++(*pmdp);
		if (unlikely((((unsigned long)*pmdp)&0xfff)==0))
		{
			++(*pudp);
			if(unlikely((((unsigned long)*pudp)&(32*8))==0||pud_none(**pudp)))
			{

				return false;
			}
			*pmdp=rdfs_get_pmd(*pudp);
		}
		if(unlikely(pmd_none(**pmdp)))
		{
			return false;
		}
		*ptep=rdfs_get_pte(*pmdp);
	}
	if(unlikely(pte_none(**ptep)))
	{
		return false;
	}

	return true;
}

bool rdfs_get_sn_from_pte(int* bdev_id,unsigned long* sn,unsigned long pte)
{
	if((pte&0xfff)!=0)
		return false;
	*sn=(pte&0x0FFFFFFFFFFF000)>>12;
	*bdev_id=(pte&0x7000000000000000)>>60;
	return true;
}

int rdfs_do_page_fault(unsigned long address)
{
	rdfs_trace();
	extern struct super_block* RDFS_SUPER_BLOCK_ADDRESS;
	struct super_block * sb=RDFS_SUPER_BLOCK_ADDRESS;
	struct rdfs_sb_info *sbi = RDFS_SB(sb);

	extern struct mm_struct init_mm;
	struct mm_struct *mm=&init_mm;

	struct inode**p_ino=rdfs_address_to_ino(sb,address);
	struct inode* inode=*p_ino;
	struct rdfs_inode_info *ni_info=RDFS_I(inode);
	struct rdfs_inode *ni = get_rdfs_inode(sb, inode->i_ino);

	int i=0;
	phys_addr_t phys;	
	unsigned long file_start_addr,file_offset_addr;
	unsigned long offset;

	pgd_t* pgd;
	pud_t* pud;
	pmd_t* pmd;
	pte_t* pte;
	address=address&(~(FAULT_ALIGN-1));	
	file_offset_addr=address;
	file_start_addr=(unsigned long)ni_info->i_virt_addr;
	offset=(file_offset_addr-file_start_addr)/PAGE_SIZE;
	pgd = pgd_offset(mm,address);
	pud = pud_offset(pgd,address);
	pmd = rdfs_pmd_offset(pud,address);
	pte = rdfs_pte_offset(pmd,address);
	if ((pte_val(*pte)&0xfff)==0)
	{
		u64 rdfs_block = pte_val(*pte)>>PAGE_SHIFT;

		rdfs_new_block(&phys,1,ni);

		int clientNo = (pte_val(*pte) & ((unsigned long long)0xff << 52)) >> 52;
		printk("page fault client no = %d\n",clientNo);
		if(clientNo < 0)
			printk("compute error,clientNo <= 0 !");
		int client_idx = clientNo;
		
		spin_lock(&ctx[client_idx].max_req_id.req_id_lock);
		u64 req_id = ctx[client_idx].max_req_id.max_req_id;
		ctx[client_idx].max_req_id.max_req_id = (ctx[client_idx].max_req_id.max_req_id+1)%64;
		printk("%s posted_req_cnt:%d max_req_id:%d\n",__FUNCTION__,posted_req_cnt,ctx[client_idx].max_req_id.max_req_id);
		spin_unlock(&ctx[client_idx].max_req_id.req_id_lock);
		//if(!is_modify)rdfs_modify_qp(client_idx);
		//is_modify =1;
		while(posted_req_cnt>=8);
		set_bit(req_id,&ctx[client_idx].max_req_id.lock_word);
		rdfs_block_rw(rdfs_block,RDFS_READ,phys,req_id,client_idx);
		posted_req_cnt++;
		//set_bit(req_id,&ctx[client_idx].max_req_id.lock_word);
		wait_on_bit_lock(&ctx[client_idx].max_req_id.lock_word,req_id,TASK_KILLABLE);
		update_swap_bufffer(ni_info,pte,offset);
		set_pte(pte,__pte(phys|(_PAGE_PRESENT | _PAGE_RW | _PAGE_GLOBAL)));
		printk("phys : %lx",phys);
		printk("pte_val: %lx",pte_val(*pte));
	}
	printk("%s  end\n",__FUNCTION__);
	return VM_FAULT_MAJOR;

}
