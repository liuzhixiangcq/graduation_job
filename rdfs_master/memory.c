/* @author page
 * @date 2017/12/3
 * rdfs/memory.c
 */

 
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
 #include "inode.h"
 #include "debug.h"
 #include "balloc.h"
 #include "memory.h"
 #include "rdfs_config.h"
 #include "network.h"
 
 extern struct slave_info slave_infos[MAX_SLAVE_NUMS];
 struct pte_mem * pte_free_list;

 int rdfs_init_slave_memory_bitmap_list(struct slave_info *s)
 {
	int block_nums = s->ctx->block_nums;
	int bitmap_nums = block_nums / 64;
	s->free_block_nums = block_nums;
	s->total_block_nums = block_nums;
	s->bitmap_head = NULL;
	int i;
	struct mem_bitmap_block *tmp = NULL;
	struct mem_bitmap_block *pre = NULL;
	for(i=0;i<bitmap_nums;i++)
	{
		tmp = (struct mem_bitmap_block*)kmalloc(sizeof(struct mem_bitmap_block),GFP_KERNEL);
		tmp->start_block_id = i * 64;
		tmp->next = NULL;
		tmp->slave_id = s->slave_id;
		//tmp->bitmap = 0;
		if(i == 0)
		{
			s->bitmap_head = tmp;
			pre = tmp;
		}
		else
		{
			pre->next = tmp;
			pre = tmp;
		}
	}
	return 0;
 }
 int rdfs_init_slave_memory_list(struct slave_info*s)
 {
	int block_nums = s->ctx->block_nums;
	s->free_block_nums = block_nums;
	s->total_block_nums = block_nums;
	s->head = NULL;
	int i;
	struct mem_block *tmp = NULL;
	struct mem_block *pre = NULL;
	for(i=0;i<block_nums;i++)
	{
		tmp = (struct mem_block*)kmalloc(sizeof(struct mem_block),GFP_KERNEL);
		tmp->block_id = i;
		tmp->next = NULL;
		if(i == 0)
		{
			s->head = tmp;
			pre = tmp;
		}
		else
		{
			pre->next = tmp;
			pre = tmp;
		}
	}
	return 0;
 }
 int rdfs_free_slave_memory_bitmap_list(struct slave_info *s)
 {
	int block_nums = s->free_block_nums;
	int bitmap_nums = block_nums / 64;
	int i;
	spin_lock(&s->slave_free_list_lock);
	struct mem_bitmap_block *tmp = s->bitmap_head;
	struct mem_bitmap_block *next = NULL;
	for(i=0;i<bitmap_nums;i++)
	{
		next = tmp->next;
		kfree(tmp);
	}
	s->bitmap_head = NULL;
	spin_unlock(&s->slave_free_list_lock);
	return 0;
 }
 int rdfs_free_slave_memory_list(struct slave_infos*s)
 {
	int block_nums = s->free_block_nums;
	int i;
	spin_lock(&s->slave_free_list_lock);
	struct mem_block *tmp = s->head;
	struct mem_block *next = NULL;
	for(i=0;i<block_nums;i++)
	{
		next = tmp->next;
		kfree(tmp);
	}
	s->head = NULL;
	spin_unlock(&s->slave_free_list_lock);
	return 0;
 }
 unsigned long rdfs_alloc_slave_bitmap(struct slave_info *s)
 {
	struct mem_bitmap_block * tmp = NULL;
	struct mem_bitmap_block * use = NULL;
	unsigned long  start_block_id ;
	spin_lock(&s->slave_free_list_lock);
	if(!s->free_block_nums)
	{
		printk("%s -- > slave memory spave not enough\n",__FUNCTION__);
		return -1;
	}
	tmp = s->bitmap_head->next;
	use = s->bitmap->head;
	s->bitmap_head = tmp;
	s->free_block_nums -= 64;
	spin_unlock(&s->slave_free_list_lock);
	start_block_id = use->start_block_id;
	kfree(use);
	return start_block_id;
 }
 unsigned long rdfs_alloc_slave_memory(struct slave_info *s)
 {
	 struct mem_block * tmp = NULL;
	 struct mem_block * use = NULL;
	 unsigned long  block_id ;
	 spin_lock(&s->slave_free_list_lock);
	 if(!s->free_block_nums)
	 {
		 printk("%s -- > slave memory spave not enough\n",__FUNCTION__);
		 return -1;
	 }
	 tmp = s->head->next;
	 use = s->head;
	 s->head = tmp;
	 spin_unlock(&s->slave_free_list_lock);
	 block_id = (unsigned long)use->block_id;
	 kfree(use);
	 return block_id;
 }
 int rdfs_free_slave_bitmap(struct slave_info *s,struct mem_bitmap_block * tmp)
 {
	//struct mem_bitmap_block * tmp = NULL;
	//tmp = (struct mem_bitmap_block*)kmalloc(sizeof(struct mem_bitmap_block),GFP_KERNEL);
	spin_lock(&s->slave_free_list_lock);
	//tmp->bitmap = 0;
	tmp->next = s->bitmap_head;
	tmp->start_block_id = start_block_id;
	s->bitmap_head = tmp;
	spin_unlock(&s->slave_free_list_lock);
	return 0;
 }
 int rdfs_free_slave_memory(struct slave_info *s,block_id)
 {
	 struct mem_block * tmp = NULL;
	 tmp = (struct mem_block*)kmalloc(sizeof(struct mem_block),GFP_KERNEL);
	 spin_lock(&s->slave_free_list_lock);
	 tmp->block_id = block_id;
	 tmp->next = s->head;
	 s->head = tmp;
	 spin_unlock(&s->slave_free_list_lock);
	 return 0;
 }

 int rdfs_init_pte_free_list(int page_nums)
 {
	 unsigned long phyaddr;
	 rdfs_new_block(&phyaddr,page_nums,NULL);
	 int pte_nums = page_nums * 4096 / 8;
	 pte_free_list = (struct pte_mem*)kmalloc(sizeof(struct pte_mem),GFP_KERNEL);
	 pte_free_list->free_pte_nums = pte_nums;
	 pte_free_list->total_page_nums = page_nums;
	 pte_free_list->pte_start_addr = phyaddr;
	 pte_free_list->start_addr = phyaddr;
	 spin_lock_init(&pte_free_list->pte_lock);
	 int i;
	 unsigned long * tmp = rdfs_va(phyaddr);
	 unsigned long next_pte_phyaddr = phyaddr + 8;
	 for(i=0;i<pte_nums;i++)
	 {
		*tmp = next_pte_phyaddr;
		tmp = rdfs_va(next_pte_phyaddr);
		next_pte_phyaddr += 8;
	 }
	 return 0;
 }
 int rdfs_free_pte_free_list()
 {
	 spin_lock(&pte_free_list->pte_lock);
	 int i;
	 phyaddr_t phyaddr = pte_free_list->start_addr;
	 for(i=0;i<pte_free_list->total_page_nums;i++)
	 {
		 rdfs_free_block(sb,phyaddr);
		 phyaddr += PAGE_SIZE;
	 }
	 spin_unlock(&pte_free_list->pte_lock);
	 kfree(pte_free_list);
	 return 0;
 }
 int rdfs_select_slave()
 {
	 return 0;
 }
 int rdfs_alloc_slave_bitmap_memory(struct rdfs_inode_info * ri_info,unsigned long *block_id,unsigned long* s_id)
 {
	rdfs_trace();
	if(ri_info == NULL)
	{
		printk("%s ri_info error\n",__FUNCTION__);
		return -1;
	}
	unsigned long start_block_id;
	struct mem_bitmap_block * tmp = NULL,*next = NULL;

	// pre_bitmap used completed
	if(ri_info->pre_bitmap->pre_index == 64)
	{
		//mv bitmap to used list
		if(ri_info->pre_bitmap->pre_alloc_slave_id != -1)
		{
			tmp = (struct mem_bitmap_block*)kmalloc(sizeof(struct mem_bitmap_block),GFP_KERNEL);
			tmp->slave_id = ri_info->pre_bitmap->pre_alloc_slave_id;
			tmp->start_block_id = ri_info->pre_bitmap->pre_start_block_id;
			tmp->next = NULL;
			spin_lock(&ri_info->used_bitmap_list_lock);
			if(ri_info->used_bitmap_list == NULL)
				ri_info->used_bitmap_list = tmp;
			else
				{
					tmp->next = ri_info->used_bitmap_list;
					ri_info->used_bitmap_list = tmp;
				}
			spin_unlock(&ri_info->used_bitmap_list_lock);
		}
		
		// re alloc a bitmap
		*s_id = rdfs_select_slave();
		struct slave_info * s = &slave_infos[*s_id];
		start_block_id = rdfs_alloc_slave_bitmap(s);
		ri_info->pre_bitmap->pre_start_block_id = start_block_id;
		ri_info->pre_bitmap->pre_alloc_bitmap = 0;
		ri_info->pre_bitmap->pre_alloc_slave_id = *s_id;
		ri_info->pre_bitmap->pre_index = 0; 
	}
	
	/* not completed*/
	// alloc a slave memory block
	spin_lock(&ri_info->bitmap_lock);
	set_bit(ri_info->pre_index,&ri_info->pre_alloc_bitmap);
	*block_id = ri_info->pre_index + ri_info->start_block_id;
	ri_info->pre_index ++;
	spin_unlock(&ri_info->bitmap_lock);
	*s_id = ri_info->pre_alloc_slave_id;
	return 0;
 }
 //not used
 int rdfs_free_slave_bitmap_memory(struct rdfs_inode_info * ri_info,unsigned long block_id)
 {
	rdfs_trace();
	if(ri_info == NULL)
	{
		printk("%s ri_info error\n",__FUNCTION__);
		return -1;
	}
	//unsigned long s_id = ri_info->pre_bitmap->slave_id;
	spin_lock(&ri_info->bitmap_lock);
	ri_info->pre_index --;
	clear_bit(ri_info->pre_index,&ri_info->pre_alloc_bitmap);
	return 0;
 }
 int rdfs_free_file_used_bitmap(struct rdfs_inode_info * ri_info)
 {
	 struct mem_bitmap_block * tmp = NULL,*next = NULL;
	 struct slave_info * s;
	 if(ri_info->used_bitmap_list)
	 {
		spin_lock(&ri_info->used_bitmap_list_lock);
		tmp = ri_info->used_bitmap_list;
		while(tmp)
		{
			next = tmp->next;
			s = &slave_infos[tmp->slave_id];
			rdfs_free_slave_bitmap(s,tmp);
			tmp = next;
		}
		spin_unlock(&ri_info->used_bitmap_list_lock);
	 }
	 if(ri_info->pre_bitmap->pre_index == 0)
	 {
		 tmp = (struct mem_bitmap_block*)kmalloc(sizeof(struct mem_bitmap_block),GFP_KERNEL);
		 tmp->slave_id = ri_info->pre_bitmap->pre_alloc_slave_id;
		 tmp->start_block_id = ri_info->pre_bitmap->pre_start_block_id;
		 tmp->next = NULL;
		 s = &slave_infos[tmp->slave_id];
		 rdfs_free_slave_bitmap(s,tmp);
	 }
	 return 0;
 }
 int rdfs_new_pte(struct rdfs_inode_info * ri_info,unsigned long *phyaddr)
 {
	 //int s_id = rdfs_select_slave();
	 int i;
	 unsigned long tmp;
	 unsigned long pte_value;
	 unsigned long s_id,block_id;
	 spin_lock(&pte_free_list->pte_lock);
	 if(pte_free_list->free_pte_nums < nums)
	 {
		 printk("%s -- pte_free_list not enogh\n",__FUNCTION__);
		 spin_unlock(&pte_free_list->pte_lock);
		 return -1;
	 }
	
	 *phyaddr = pte_free_list->pte_start_addr;
	 
	 tmp = pte_free_list->pte_start_addr;
	 pte_free_list->pte_start_addr = *(rdfs_va(tmp));
	 spin_unlock(&pte_free_list->pte_lock);

	 rdfs_alloc_slave_bitmap_memory(ri_info,&block_id,&s_id);
	 pte_value = (s_id << SLAVE_ID_SHIFT) & block_id; 
	 *(rdfs_va(tmp)) = pte_value;
	 
	 return 0;
 }
 int rdfs_free_pte(struct rdfs_inode_info * ri_info,unsigned long phyaddr)
 {
	 int s_id;
	 int block_id;
	 unsigned long pte_val = *(rdfs_va(phyaddr));
	 s_id = pte_val >> SLAVE_ID_SHIFT;
	 block_id = pte_val & SLAVE_ID_MASK;
	 struct slave_info *s = &slave_infos[s_id];
	 //not used
	 //rdfs_free_slave_bitmap_memory(ri_info,s,block_id);
	 spin_lock(&pte_free_list->pte_lock);
	 *rdfs_va(phyaddr) = pte_free_list->pte_start_addr;
	 pte_free_list->pte_start_addr = phyaddr;
	 spin_unlock(&pte_free_list->pte_lock);
	 return 0;
 }
 