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
#define LIST_NUM 3

struct slave_info **slave_mem_info = NULL;
unsigned long malloc_flag = LIST_NUM-1;
unsigned long size_array[4];

unsigned long path_to_inode(char *path)
{
	rdfs_trace();
    struct inode *inode = NULL;
    struct file *file=NULL;
    file=filp_open(path,O_DIRECTORY,0);
    printk("\n\n********************\n\n");
    if(IS_ERR(file))
    {
     		file=filp_open(path,O_RDONLY,0444);
        	if(IS_ERR(file))
 		{
     			printk("The path %s is error!\n",path);
 			return 0;
 		}
    }
    inode = file->f_inode;
    printk("This is a file , show my information : \n");
    printk("1.inode number : %ld\n",inode->i_ino);
    return inode->i_ino;
}
void rdfs_init_mem_node(struct mem_node *head_node,unsigned long  size)
{
	rdfs_trace();
	head_node->pre_node  = NULL;
	head_node->next_node = NULL;
	head_node->mem_size  = size;
}
void rdfs_add_mem_to_list(unsigned long slave_id,unsigned long index,unsigned long size,unsigned long phy_addr)
{
	rdfs_trace();
	unsigned long offset     = size_array[index];
	unsigned long block_num  = size>>offset;
	unsigned long totoalsize = 1<<offset;
	struct mem_node *head_node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
	struct mem_node **free_list = slave_mem_info[slave_id]->free_list;
	int i = 0;
	rdfs_init_mem_node(head_node,totoalsize);
	head_node->cur_addr = phy_addr;
	free_list[index] = head_node;
	phy_addr += FK_SIZE;
	for(i;i<block_num;i++)
	{
		struct mem_node *node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
		head_node->next_node = node;
		node->pre_node       = head_node;
		node->cur_addr       = phy_addr;
		node->next_node      = NULL;
		node->mem_size       = totoalsize;
		head_node         	 = node;
		phy_addr += totoalsize;
	}
}

unsigned long rdfs_split_mem_from_toplist(unsigned long slave_id,unsigned long index)
{
	rdfs_trace();
	if(index>3)
		return -1;
	int res = 1;
	struct mem_node **free_list = slave_mem_info[index]->free_list;
	if(free_list[index]==NULL)
	{
		if(index==3)
			return 0;
		res = rdfs_split_mem_from_toplist(slave_id,index+1);
		return res;
	}
	struct mem_node *node = free_list[index];
	free_list[index] = node->next_node;
	node->pre_node  = NULL;
	rdfs_add_mem_to_list(slave_id,index-1,node->mem_size,node->cur_addr);
	kfree(node);
	return res;
}

void rdfs_split_mem(unsigned long slave_id,unsigned long smallpersent,unsigned long largepersent)
{
	rdfs_trace();
	unsigned long total_size = slave_mem_info[slave_id]->mem_size;
	unsigned long fk_size    = (total_size/100*5)>>12;
	unsigned long tm_size    = (total_size/100*smallpersent)>>21;
	unsigned long otm_size   = (total_size/100*largepersent)>>27;
	unsigned long og_size    = (total_size/100*5)>>30;
	unsigned long phy_addr   = slave_mem_info[slave_id]->phy_addr;
	printk("total size : %lu MB\n",(total_size / 1024 / 1024));
	printk("%lu list  fksize %lu tmsize %lu otemsize %lu ogsize %lu,phy_addr 0x%lx\n",slave_id,fk_size,tm_size,otm_size,og_size,phy_addr);
	struct mem_node **free_list = slave_mem_info[slave_id]->free_list;
	int i = 1;
	struct mem_node *fk_head_node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
	struct mem_node *tm_head_node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
	struct mem_node *otm_head_node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
	struct mem_node *og_head_node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
	rdfs_init_mem_node(fk_head_node,FK_SIZE);
	rdfs_init_mem_node(tm_head_node,TM_SIZE);
	rdfs_init_mem_node(otm_head_node,OTM_SIZE);
	rdfs_init_mem_node(og_head_node,OG_SIZE);
	fk_head_node->cur_addr = phy_addr;
	free_list[0] = fk_head_node;
	free_list[1] = tm_head_node;
	free_list[2] = otm_head_node;
	free_list[3] = og_head_node;
	phy_addr += FK_SIZE;
	for(i;i<fk_size;i++)
	{
		struct mem_node *node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
		fk_head_node->next_node = node;
		node->pre_node       = fk_head_node;
		node->cur_addr       = phy_addr;
		node->next_node      = NULL;
		node->mem_size       = FK_SIZE;
		fk_head_node         = node;
		phy_addr += FK_SIZE;
	}
	i = 1;
	tm_head_node->cur_addr = phy_addr;
	phy_addr +=TM_SIZE;
	for(i;i<tm_size;i++)
	{
		struct mem_node*node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
		tm_head_node->next_node = node;
		node->pre_node       = tm_head_node;
		node->cur_addr       = phy_addr;
		node->next_node      = NULL;
		node->mem_size       = TM_SIZE;
		tm_head_node         = node;
		phy_addr += TM_SIZE;
	}
	i = 1;
	otm_head_node->cur_addr = phy_addr;
	phy_addr +=OTM_SIZE;
	for(i;i<otm_size;i++)
	{
		struct mem_node*node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
		otm_head_node->next_node = node;
		node->pre_node       = otm_head_node;
		node->cur_addr       = phy_addr;
		node->next_node      = NULL;
		node->mem_size       = OTM_SIZE;
		otm_head_node         = node;
		phy_addr += OTM_SIZE;
	}
	i = 1;
	og_head_node->cur_addr = phy_addr;
	phy_addr +=OG_SIZE;
	for(i;i<otm_size;i++)
	{
		struct mem_node*node = (struct mem_node*)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
		og_head_node->next_node = node;
		node->pre_node       = og_head_node;
		node->cur_addr       = phy_addr;
		node->next_node      = NULL;
		node->mem_size       = OG_SIZE;
		og_head_node         = node;
		phy_addr += OG_SIZE;
	}
}
void rdfs_init_freelist(unsigned long slave_id,unsigned long mem_size,unsigned long phy_addr)
{
	rdfs_trace();
	if(slave_mem_info==NULL)
	{
		slave_mem_info = (struct slave_info**)kmalloc(3*sizeof(struct slave_info*),GFP_KERNEL);
	}
	size_array[0] = 12;
	size_array[1] = 21;
	size_array[2] = 27;
	size_array[3] = 30;
	slave_mem_info[slave_id] = (struct slave_info*)kmalloc(sizeof(struct slave_info),GFP_KERNEL);
	slave_mem_info[slave_id]->mem_size = mem_size;
	slave_mem_info[slave_id]->phy_addr = phy_addr;
	slave_mem_info[slave_id]->slave_id = slave_id;
	slave_mem_info[slave_id]->cur_mem_size = mem_size;
	slave_mem_info[slave_id]->free_list = (struct mem_node**)kmalloc(4*sizeof(struct mem_node*),GFP_KERNEL);
	//printk("rdfs_init_freelist slave_id = %ld phy_addr 0x%lx\n",slave_id,phy_addr);
	rdfs_split_mem(slave_id,45,45);
}

unsigned long rdfs_get_new_block(unsigned long slave_id,unsigned long size,struct mem_node *block)
{
	rdfs_trace();
	unsigned long res_addr = 0;
	struct mem_node **free_list = slave_mem_info[slave_id]->free_list;
	int index = 0;
	if(size<=FK_SIZE)
	{
		index = 0;
        	block->mem_size = FK_SIZE;
	}
	else if(size<=TM_SIZE)
	{
		index = 1;
		block->mem_size = TM_SIZE;
	}
	else if (size<=OTM_SIZE)
	{
		index = 2;
		block->mem_size = OTM_SIZE;
	}
	else if(size<=OG_SIZE)
	{
		index = 3;
		block->mem_size = OG_SIZE;
	}
	else
	{
		printk("rdfs_get_new_block you need memory > 1G.please readjust mem manage.\n");
		return res_addr;
	}
	//printk("free_list[index]:%lx index:%ld\n",free_list[index],index);
	if(free_list[index])
	{
		//printk("here 1\n");
		struct mem_node *head_node = free_list[index];
		res_addr = head_node->cur_addr;
		free_list[index] = head_node->next_node;
		kfree(head_node);
	}
	else
	{
		printk("here 2  index = %d \n",index);
		res_addr = rdfs_split_mem_from_toplist(slave_id,index+1);
		if(!res_addr)
		{
			return res_addr;
		}
		struct mem_node *head_node = free_list[index];
		res_addr = head_node->cur_addr;
		free_list[index] = head_node->next_node;
		kfree(head_node);
	}
	//printk("rdfs_get_new_block slave_id %lu ,addr 0x%lx\n",slave_id,res_addr);
	return res_addr;
}

unsigned long rdfs_new_block_update(unsigned long size,struct mem_node *block)
{
	rdfs_trace();
	int circle_size = 0;
	unsigned long slave_id;
	unsigned long phy_addr = 0;
	while(phy_addr==0&&circle_size<4)
	{
		slave_id = 0;
		phy_addr = rdfs_get_new_block(slave_id,size,block);
		if(phy_addr!=0)
			break;
		circle_size++;
	}
	block->cur_addr = phy_addr;
	block->slave_id = slave_id;
	return slave_id;
}

void rdfs_free_blcok(unsigned long size,unsigned long phy_addr,unsigned slave_id)
{
	rdfs_trace();
	struct mem_node **free_list = slave_mem_info[slave_id]->free_list;
	struct mem_node *node = (struct mem_node *)kmalloc(sizeof(struct mem_node),GFP_KERNEL);
	struct mem_node *tmp_node;
	rdfs_init_mem_node(node,size);
	node->cur_addr = phy_addr;
	if(size==FK_SIZE)
	{
		tmp_node = free_list[0];
		free_list[0] = node;
	}
	if(size==TM_SIZE)
	{
		tmp_node = free_list[1];
		free_list[1] = node;
	}
	if(size==OTM_SIZE)
	{
		tmp_node = free_list[2];
		free_list[2] = node;
	}
	if(size==OG_SIZE)
	{
		tmp_node = free_list[3];
		free_list[3] = node;
	}
	tmp_node->pre_node = node;
	node->next_node = tmp_node;
	//printk("recycle node 0x%lx\n",node->cur_addr);
}

void dmfs_index_init(struct rdfs_inode_info *inode)
{
	rdfs_trace();
	inode->index = kmalloc(sizeof(struct index_struct) * MAX_BLOCK_NUM,GFP_KERNEL);
    	inode->blocks = kmalloc(sizeof(struct mem_node) * MAX_BLOCK_NUM,GFP_KERNEL);
	inode->block_num = 0;
	//printk("%s over\n",__FUNCTION__);
}

void dmfs_index_free(struct rdfs_inode_info *inode)
{
	rdfs_trace();
	kfree(inode->index);
	kfree(inode->blocks);
}

void dmfs_index_update(struct rdfs_inode_info *inode)
{
    rdfs_trace();
    if(inode->block_num == 1)
    {
        inode->index[0].sum = inode->blocks[0].mem_size;
	inode->file_size = inode->blocks[0].mem_size;

	i_size_write(&inode->vfs_inode, inode->file_size);
    }
    else if(inode->block_num > 1)
    {
		inode->index[inode->block_num - 1].sum = inode->index[inode->block_num - 2].sum + inode->blocks[inode->block_num - 1].mem_size;
		
	inode->file_size += inode->blocks[inode->block_num - 1].mem_size;

	i_size_write(&inode->vfs_inode, inode->file_size);
    }
}

int dmfs_new_block(struct rdfs_inode_info *inode,unsigned long size)
{
    rdfs_trace();
    if(inode->block_num < MAX_BLOCK_NUM)
    {
        rdfs_new_block_update(size,inode->blocks + inode->block_num);
        inode->block_num += 1;
		dmfs_index_update(inode);
    }
    else{
	return -1;
    }
    return 0;
}

unsigned long dmfs_block_search(struct rdfs_inode_info *inode,unsigned long key)
{
    rdfs_trace();
    signed long left = 0;
    signed long right = inode->block_num - 1;
    //printk("left= %ld , right = %ld \n",left,right);
    while(left <= right)
    {
        signed long mid = (left + right) / 2;
	//printk("mid = %ld  sum = %ld  key = %ld\n",mid,inode->index[mid].sum,key);
        if(inode->index[mid].sum < key)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }
    //printk("left = %ld , inode->block_num = %ld \n",left,inode->block_num);
    if(left < inode->block_num && inode->index[left].sum >= key)
    {
        //printk("key = %ld  block no: %ld  prefix sum: %ld\n",key,left,inode->index[left].sum);
        return left;
    }
}

unsigned long dmfs_index_search(struct rdfs_inode_info *inode,loff_t offset,unsigned long size,struct index_search_result *result)
{
    rdfs_trace();
    unsigned long a = dmfs_block_search(inode,offset);
    if(inode->index[a].sum == offset)
    {
        a += 1;
    }
    unsigned long b = dmfs_block_search(inode,offset + size);
    printk("--------------------> a = %d , b = %d \n",a,b);
    result[0].slave_id = inode->blocks[a].slave_id;
    unsigned long temp_offset = inode->blocks[a].mem_size - (inode->index[a].sum - offset);
    result[0].start_addr = inode->blocks[a].cur_addr + temp_offset;
    if(a == b)
        result[0].size = size;
    else
        result[0].size = inode->index[a].sum - offset;
    result[0].block_type = inode->blocks[a].mem_size;
    unsigned long i = 0;
    result[0].file_offset = 0;
    for(i = 0; i < a; i++)
    {
    	result[0].file_offset += inode->blocks[i].mem_size;
    }
    if(b > a)
    {
        for(i = a + 1;i <= b;i++)
        {
            result[i - a].slave_id = inode->blocks[i].slave_id;
            result[i - a].start_addr = inode->blocks[i].cur_addr;
            result[i - a].block_type = inode->blocks[i].mem_size;
            if(i != b)
                result[i - a].size = inode->blocks[i].mem_size;
            else
                result[i - a].size = inode->blocks[i].mem_size - (inode->index[i].sum - offset - size);
	    result[i - a].file_offset = result[i - a - 1].file_offset + result[i - a - 1].block_type;
        }
    }
    printk("------------------>\n");
    for(i = 0; i < (b-a+1); i++)
    {
	printk("%ld\t%ld\t%ld\t%ld\t%ld\n",result[i].slave_id,result[i].start_addr,result[i].block_type,result[i].size,result[i].file_offset);
    }
    return b-a+1;
}
