#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/bio.h>
#include <linux/rwlock.h>

#include <asm/barrier.h>
#include <asm/tlbflush.h>
#include <linux/delay.h>

#include "rdfs.h"
#include "server.h"
#include "balloc.h"
#include "debug.h"
#include "inode.h"
#include "rdfs_lru_op.h"
#include "pgtable.h"
#define CLEAN 1
#define NO_CLEAN 0
#define LOCK 1
#define NO_LOCK 0
#define MAX_THRESHOLD 0xFFFFFFFF
extern int ctx_cnt;
int current_client = 0;
extern int is_modify;
extern int posted_req_cnt;
extern struct rdfs_context ctx[MAX_CLIENT_NUMS];
struct rdfs_inode* get_first_openning_file(struct numa_des* numa_des,struct rdfs_inode_info** p_ni_info)
{
	rdfs_trace();
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	struct inode* inode = NULL;
	struct rdfs_inode* ret;

	*p_ni_info =NULL;
	ret = rdfs_get_first_node(numa_des);
	while(ret!=NULL)
	{
		inode = rdfs_iget(sb,get_rdfs_inodenr(sb,rdfs_pa(ret)));
		*p_ni_info = RDFS_I(inode);
		if (atomic_read(&(*p_ni_info)->i_p_counter) != 0)
		{
			return ret;
		}
		ret = rdfs_get_next_node(ret);
	}
	return ret;
}

struct rdfs_inode* get_next_openning_file(struct numa_des* numa_des,struct rdfs_inode_info** p_ni_info,struct rdfs_inode* ni)
{
	rdfs_trace();
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	struct inode* inode = NULL;
	struct rdfs_inode* ret = rdfs_get_next_node(ni);

	while(ret!=NULL)
	{
		inode = rdfs_iget(sb,get_rdfs_inodenr(sb,rdfs_pa(ret)));
		*p_ni_info = RDFS_I(inode);
		if (atomic_read(&(*p_ni_info)->i_p_counter) != 0)
		{
			return ret;
		}
		ret = rdfs_get_next_node(ret);
	}
	return ret;
}

struct rdfs_inode* get_first_closed_file(struct numa_des* numa_des,struct rdfs_inode_info** p_ni_info)
{
	rdfs_trace();
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	struct inode* inode = NULL;
	struct rdfs_inode* ret;

	*p_ni_info =NULL;
	ret = rdfs_get_first_node(numa_des);
	while(ret!=NULL)
	{
		inode = rdfs_iget(sb,get_rdfs_inodenr(sb,rdfs_pa(ret)));
		*p_ni_info = RDFS_I(inode);
		if (atomic_read(&(*p_ni_info)->i_p_counter) == 0)
		{
			return ret;
		}
		ret = rdfs_get_next_node(ret);
	}
	return ret;
}

struct rdfs_inode* get_next_closed_file(struct numa_des* numa_des,struct rdfs_inode_info** p_ni_info,struct rdfs_inode* ni)
{
	rdfs_trace();
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	struct inode* inode = NULL;
	struct rdfs_inode* ret = rdfs_get_next_node(ni);

	while(ret!=NULL)
	{
		inode = rdfs_iget(sb,get_rdfs_inodenr(sb,rdfs_pa(ret)));
		*p_ni_info = RDFS_I(inode);
		if (atomic_read(&(*p_ni_info)->i_p_counter) == 0)
		{
			return ret
;
		}
		ret = rdfs_get_next_node(ret);
	}
	return ret;
}

void rdfs_swap_buffer_init(struct swap_buffer* buf)
{
	rdfs_trace();
	buf->file_pages = __MAX_FILE_SIZE>>PAGE_SHIFT;
	buf->pte =  vmalloc(sizeof(pte_t*)*(buf->file_pages));
	buf->sn_num = vmalloc(sizeof(u64)*(buf->file_pages));
	buf->accessed_count = vmalloc(sizeof(unsigned long)*(buf->file_pages));
	
	memset(buf->pte,0,sizeof(pte_t*)*(buf->file_pages));
	memset(buf->sn_num,0,sizeof(sector_t)*(buf->file_pages));
	memset(buf->accessed_count,0,sizeof(int)*(buf->file_pages));

	spin_lock_init(&buf->lock);
}

void update_swap_bufffer(struct rdfs_inode_info* info,pte_t* pte,unsigned long offset)
{
	rdfs_trace();
	struct swap_buffer* buffer=&info->swap_buffer;
	buffer->pte[offset]=pte;
	buffer->sn_num[offset]=pte_val(*pte);
}

unsigned long swap_threshold(struct rdfs_inode* ni)//ni or ni_info?
{
	rdfs_trace();
	if (ni->i_blocks == 0)
	{
		return MAX_THRESHOLD;
	}
	else
	{
		return ni->i_accessed_count/ni->i_blocks;
	}	
}

void clear_swap_buffer_accessed(struct rdfs_inode_info* ni_info)
{
	rdfs_trace();
	struct swap_buffer* buffer=&ni_info->swap_buffer;
	memset(buffer->accessed_count,0,sizeof(unsigned long)*buffer->file_pages);
}
int lock_count = 0;
unsigned long rdfs_swap_out(pte_t *pte,int client_idx)
{
	rdfs_trace();
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	phys_addr_t phys; 
	u64 rdfs_block;
	phys=rdfs_get_phy_from_pte(pte);

	rdfs_block = rdfs_new_client_block(client_idx);

	spin_lock(&ctx[client_idx].max_req_id.req_id_lock);
	u64 req_id = ctx[client_idx].max_req_id.max_req_id;
	ctx[client_idx].max_req_id.max_req_id = (ctx[client_idx].max_req_id.max_req_id+1)%64;
	spin_unlock(&ctx[client_idx].max_req_id.req_id_lock);
	while(posted_req_cnt>=8);
	set_bit(req_id,&ctx[client_idx].max_req_id.lock_word);
	rdfs_block_rw(rdfs_block,RDFS_WRITE,phys,req_id,client_idx);
	posted_req_cnt++;
	wait_on_bit_lock(&ctx[client_idx].max_req_id.lock_word,req_id,TASK_KILLABLE);
	lock_count += 1;
	rdfs_free_block(sb,phys);
	set_pte(pte,__pte(rdfs_block<<PAGE_SHIFT));
	set_pte(pte,__pte(pte_val(*pte) | ((unsigned long long)(client_idx) << 52)));

	return 1;
}

unsigned long rdfs_swap_pte_range(pmd_t* pmd,unsigned long *start,unsigned long *size,unsigned long threshold,struct swap_buffer* swap_buffer,int client_idx)
{
	rdfs_trace();
	pte_t *pte;
	unsigned long swap_count = 0;
	unsigned long end= (*start + PMD_SIZE)&PMD_MASK;
	unsigned long offset;
	pte = rdfs_pte_offset(pmd,*start);

	while(!pte_none(*pte) && *start<end && *size)
	{	
		offset=(*start)>>PAGE_SHIFT;
		if(!pte_is_sn(pte))
		swap_count += rdfs_swap_out(pte,client_idx);

	   	++pte;
		*start+=PAGE_SIZE;
		*size = *size -PAGE_SIZE;
	}
	return swap_count;
}

unsigned long swap_pmd_range(pud_t* pud,unsigned long *start,unsigned long *size,unsigned long threshold, struct swap_buffer* swap_buffer,int client_idx)
{
	rdfs_trace();
	pmd_t *pmd;
	unsigned long swap_count = 0;
	unsigned long end = (*start + PUD_SIZE)&PUD_MASK;

	pmd=rdfs_pmd_offset(pud,*start);

	while(!pmd_none(*pmd)&&*start<end&&*size)
	{
		swap_count += rdfs_swap_pte_range(pmd,start,size,threshold,swap_buffer,client_idx);
		++pmd;
	}

	return swap_count;
}

unsigned long swap_pud_range(pud_t* pud,unsigned long *start,unsigned long *size,unsigned long threshold,struct swap_buffer* swap_buffer,int client_idx)
{
	rdfs_trace();
	unsigned long swap_count = 0;
	pud += pud_index(*start);

	*start = *start&PAGE_MASK;
	*size  = (*size + PAGE_SIZE -1)&PAGE_MASK;

	while(!pud_none(*pud)&&*size)
	{
		swap_count += swap_pmd_range(pud,start,size,threshold,swap_buffer,client_idx);
		++pud;
	}

	return swap_count;
}

unsigned long swap_file_NVM(struct rdfs_inode* ni,struct rdfs_inode_info* ni_info,int clean,int lock,int client_idx)
{	
	rdfs_trace();
	int swap_count=0;
	unsigned long threshold = 0;
	unsigned long start = 0;
	unsigned long end;
	unsigned long size;
	pud_t * pud;
	struct swap_buffer* swap_buffer;

	if(ni==NULL||ni->i_size<start)
	{
		return swap_count;
	}
	end = (ni->i_blocks)<<PAGE_SHIFT;
	pud = (pud_t*)rdfs_va(ni->i_pg_addr);
	if (clean == CLEAN)
	{
		threshold = MAX_THRESHOLD;
	}
	else
	{
		threshold = swap_threshold(ni);
	}

	swap_buffer = & ni_info->swap_buffer;

	if (lock==LOCK&&!write_trylock(&ni_info->state_rwlock))
	{
//		printk("%s can not get lock\n",__FUNCTION__);
		return swap_count;
	}
	

	size = end - start;

	swap_count = swap_pud_range(pud,&start,&size,threshold,swap_buffer,client_idx);

	flush_cache_all();

	if (lock==LOCK)
	{
		write_unlock(&ni_info->state_rwlock);
	}
	return swap_count;
}
EXPORT_SYMBOL(swap_file_NVM);

unsigned long update_openning_hot_page(struct rdfs_inode* ni,struct rdfs_inode_info* ni_info)
{	
	rdfs_trace();
	pud_t *pud;
    pmd_t *pmd;
    pte_t *pte; 
	int count=0;
	int page_num=0;
	int offset=0;
	int start_addr=0;
	int access=0;
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	struct inode* inode = NULL;
	inode = rdfs_iget(sb,get_rdfs_inodenr(sb,rdfs_pa(ni)));
	//struct inode *inode=ni_info->vfs_inode;
	int ino = inode->i_ino;
	//struct super_block *sb = inode->i_sb;
	unsigned char *cold_dram;
	unsigned int *hot_dram;
	cold_dram=ni->i_cold_dram;
	hot_dram=ni->i_hot_dram;
	while(count<5){
		if(hot_dram[count]!=0){
			page_num=hot_dram[count];
			offset=page_num*4096;
			start_addr = RDFS_I(inode)->i_virt_addr+offset;
			pud=rdfs_pud_alloc(sb,ino,offset);
			pmd=rdfs_pmd_offset(pud,start_addr);
			pte=rdfs_pte_offset(pmd,start_addr);
			access=(pte_val(*pte) & 0x0000000000000020)>>5;

			if(access==0){
				set_pte(pte,__pte(pte_val(*pte)&0xfffffffffffffdff));
				hot_dram[count]=0;
				cold_dram[page_num]=1;
				

			}else{


			}

			count++;
			
		}else{
			count++;
		}

	}
	printk("\t\t%s end\n",__FUNCTION__);
	return 1;

}

unsigned long swap_openning_cold_page(struct rdfs_inode* ni,struct rdfs_inode_info* ni_info,unsigned long count,int client_idx)
{	
	rdfs_trace();
	pud_t *pud;
    pmd_t *pmd;
    pte_t *pte; 
	int swap_count=0;
	int page_num=0;
	int offset=0;int start_addr=0;
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	struct inode* inode = NULL;
	inode = rdfs_iget(sb,get_rdfs_inodenr(sb,rdfs_pa(ni)));
	//struct inode *inode=ni_info->vfs_inode;
	int ino = inode->i_ino;
	//struct super_block *sb = inode->i_sb;
	unsigned char *cold_dram;
	unsigned int *hot_dram;
	cold_dram=ni->i_cold_dram;
	hot_dram=ni->i_hot_dram;
	if(ni->i_cold_page_num==0)
		return 0;
	page_num=ni->i_cold_page_start;
	while(ni->i_cold_page_num>0&&page_num<8*1024*1024){
		if(cold_dram[page_num]==1){
			offset=page_num*4096;
			start_addr = RDFS_I(inode)->i_virt_addr+offset;
			pud=rdfs_pud_alloc(sb,ino,offset);
			if(pud_none(*pud))
				break;
			pmd=rdfs_pmd_offset(pud,start_addr);
			if(pmd_none(*pmd))
				break;
			pte=rdfs_pte_offset(pmd,start_addr);
			if(pte_none(*pte))
				break;

			if(pud_none(*pud)||pmd_none(*pmd)||pte_none(*pte))
				break;
			swap_count += rdfs_swap_out(pte,client_idx);
			cold_dram[page_num]=0;
			ni->i_cold_page_num--;
			if (count < swap_count)
			{	
				ni->i_cold_page_start = page_num+1;
				return swap_count;
			}		
			page_num++;
		}else{
			page_num++;
		}
	}
	ni->i_cold_page_num=0;//
	ni->i_cold_page_start=0;//
	return swap_count;

}

unsigned long swap_openning_file(struct numa_des* numa_des, unsigned long count)
{
	rdfs_trace();
	struct rdfs_inode* selected_file = NULL;
	struct rdfs_inode* temp_ni= NULL;
	unsigned long swap_count = 0 ;
	//unsigned long per_swap_count = 0 ;
	unsigned long temp_count = count ;
	struct rdfs_inode_info* ni_info;
	int client_idx = 0;
	int k = 1;
	int max_free_blocks = 0;
	int temp = 0;

	selected_file = get_first_openning_file(numa_des,&ni_info);
	printk("selected_file : %d",selected_file);
	while(count>swap_count&&selected_file!=NULL)
	{
		temp_ni=selected_file;

		client_idx = 0;
		max_free_blocks = rdfs_get_rm_free_page_num(&ctx[0].rrm);
		for(k = 1; k < ctx_cnt; k ++)
		{
			temp = rdfs_get_rm_free_page_num(&ctx[k].rrm);
			if(temp > max_free_blocks)
			{
				max_free_blocks = temp;
				client_idx = k;
			}
		}
		printk("choose client %d to swap data\n",client_idx);
		current_client = client_idx;
		
		swap_count += swap_openning_cold_page(selected_file,ni_info,temp_count,client_idx);
		
		update_openning_hot_page(selected_file,ni_info);
		selected_file = get_next_openning_file(numa_des,&ni_info,selected_file);
		if(temp_ni->i_cold_page_num<=0){
			rdfs_flush_list(temp_ni);
		}
		temp_count -=swap_count;
	}
	printk("\t\t%s end\n",__FUNCTION__);
	return swap_count;
}


unsigned long swap_closed_file_rdfs(struct numa_des* numa_des, unsigned long count, int clean)
{
	rdfs_trace();
	struct rdfs_inode* selected_file = NULL;
	struct rdfs_inode* temp_ni = NULL;
	unsigned long swap_count = 0 ;
	struct rdfs_inode_info* ni_info;
	selected_file = get_first_closed_file(numa_des,&ni_info);
	int client_idx = 0;
	int k = 1;
	int max_free_blocks = 0;
	int temp = 0;

	while(count>swap_count&&selected_file!=NULL)
	{
		client_idx = 0;
		max_free_blocks = rdfs_get_rm_free_page_num(&ctx[0].rrm);
		for(k = 1; k < ctx_cnt; k ++)
		{
			temp = rdfs_get_rm_free_page_num(&ctx[k].rrm);
			if(temp > max_free_blocks)
			{
				max_free_blocks = temp;
				client_idx = k;
			}
		}
		printk("choose client %d to swap data\n",client_idx);
		
		current_client = client_idx;
		temp_ni=selected_file;
		swap_count += swap_file_NVM(selected_file,ni_info,clean,LOCK,client_idx);
		selected_file = get_next_closed_file(numa_des,&ni_info,selected_file);
		rdfs_flush_list(temp_ni);
	}
	return swap_count;
}

unsigned long swap_closed_file(struct numa_des* numa_des,unsigned long count)
{
	rdfs_trace();
	unsigned long swap_count = 0;

	if (count < swap_count)
	{
		return swap_count;
	}

	swap_count += swap_closed_file_rdfs(numa_des,count - swap_count,CLEAN);
	return swap_count;
}

unsigned long swap_oneself(struct rdfs_inode* ni,unsigned long count)
{
	rdfs_trace();
	struct super_block* sb = RDFS_SUPER_BLOCK_ADDRESS;
	unsigned long swap_count = 0;
	//unsigned long swap_count = count;
	int k = 1;
	int max_free_blocks = 0;
	int client_idx = 0;
	int temp = 0;
	struct inode* inode = rdfs_iget(sb,get_rdfs_inodenr(sb,rdfs_pa(ni)));
	struct rdfs_inode_info* ni_info = RDFS_I(inode);

		max_free_blocks = rdfs_get_rm_free_page_num(&ctx[0].rrm);
		for(k = 1; k < ctx_cnt; k ++)
		{
			temp = rdfs_get_rm_free_page_num(&ctx[k].rrm);
			if(temp > max_free_blocks)
			{
				max_free_blocks = temp;
				client_idx = k;
			}
		}
		current_client = client_idx;

	swap_count += swap_openning_cold_page(ni,ni_info,count,client_idx);
	if(ni->i_cold_page_num<=0){
		rdfs_flush_list(ni);
	}

	return swap_count;
}

unsigned long rdfs_swap(struct numa_des* numa_des, struct rdfs_inode* ni,unsigned long count)
{
	rdfs_trace();
	unsigned long swap_count=0;
	swap_count += swap_closed_file(numa_des,count-swap_count);
	printk("closed_swap swap_count:%lx\n",swap_count);

	if(swap_count>count)
	{
			return swap_count;
	}
	if (ni!=NULL)
	{
		swap_count += swap_oneself(ni,count - swap_count);
	}
	printk("oneself_swap swap_count:%lx\n",swap_count);
	
	if (swap_count>count)
	{
		return swap_count;
	}
	swap_count += swap_openning_file(numa_des,count-swap_count); 
	printk("openning_swap swap_count:%lx\n",swap_count);
	if (swap_count>count)
	{
		return swap_count;
	}

	return swap_count;
}
