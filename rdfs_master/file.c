/* @author page
 * @date 2017/12/3
 * rdfs/file.c
 */


#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/aio.h>
#include <linux/slab.h>
#include <linux/uio.h>
#include <linux/mm.h>
#include <linux/uaccess.h>

#include <linux/delay.h>

#include "rdfs.h"
#include "server.h"
#include "acl.h"
#include "xip.h"
#include "xattr.h"
#include "inode.h"
#include "pgtable.h"
#define CLEAN 1
#define NO_CLEAN 0
#define LOCK 1
#define NO_LOCK 0
#define iov_iter_rw(i)((0 ? (struct iov_iter*)0: (i))->type & RW_MASK)


static int rdfs_open_file(struct inode *inode, struct file *filp)
{
	rdfs_trace();
	int errval = 0;
	struct rdfs_inode* nv_i = get_rdfs_inode(inode->i_sb,inode->i_ino);
	filp->f_flags |= O_DIRECT;
	if(S_ISREG(inode->i_mode))
		atomic_inc(&(RDFS_I(inode)->i_p_counter));
	return generic_file_open(inode, filp);
}


static int rdfs_release_file(struct file * file)
{
	rdfs_trace();
        struct inode *inode = file->f_mapping->host;
	struct rdfs_inode_info *ni_info;
	unsigned long vaddr;
	int err = 0;
	struct rdfs_inode* ni = get_rdfs_inode(inode->i_sb,inode->i_ino);
	return err;
}

inline void new_per_read(struct rdfs_inode_info* ni_info,ssize_t length)
{
	rdfs_trace();
}

static int rdfs_rw_pte_hc(struct inode *inode,size_t length,loff_t offset)
{	
	rdfs_trace();
	struct rdfs_inode_info *ni_info;
	ni_info = RDFS_I(inode);
	struct rdfs_inode *ni;
	ni=ni_info->rdfs_inode;
	int page_hot;
	int access;
	int page_start;
	int page_end;
	pud_t *pud;
   	 pmd_t *pmd;
    	pte_t *pte; 
	int i=0;
	unsigned char *cold_dram;
	unsigned int *hot_dram;
	cold_dram=ni->i_cold_dram;
	hot_dram=ni->i_hot_dram;
	struct super_block *sb = inode->i_sb;
	int ino = inode->i_ino;
	void *start_addr = RDFS_I(inode)->i_virt_addr+offset;
	page_start=offset/4096;
	page_end=(offset+length-1)/4096;
	//printk("************offset*********:%d\n",offset);
	//printk("************start_addr*********:%d\n",start_addr);
	//printk("************length*********:%d\n",length);
	//printk("************page_start*********:%d\n",page_start);
	//printk("************page_end*********:%d\n",page_end);

	while(page_start<=page_end){
		//�����ݻ���δ���������ݣ���ʱû��������ҳ�����Ը�ҳһ������������ҳ
		if(ni->i_cold_page_num==0){
			//printk("************ni->i_cold_page_num==0*********:\n");
			//δ���������ݣ��������ݶ�������
			if(ni->i_hot_page_num==0){
				//printk("************ni->i_hot_page_num==0*********:\n");
				cold_dram[page_start]=1;
				ni->i_cold_page_start=page_start;
				ni->i_cold_page_num++;
			//���������ݣ�������������	
			}else{	
				//printk("************ni->i_hot_page_num  !=0*********:\n");
				for(i=0;i<5;i++){
					if(hot_dram[i]==page_start)
						break;
				}
				//��ҳ����������ҳ
				if(i==5){
					cold_dram[page_start]=1;
					ni->i_cold_page_start=page_start;
					ni->i_cold_page_num++;
				}
				//else����������ݣ�ʲôҲ����
			}	
		}else{
		//����������ҳ
			//��ҳ����������ҳ
			//printk("************ni->i_cold_page_num  !=0*********:\n");
			if(cold_dram[page_start]!=1){
				//������������ҳ����ҳ���������ݣ�Ҳ���������ݣ���δ������ҳ
				//printk("************cold_dram[page_start]==0*********:\n");
				if(ni->i_hot_page_num==0){
					//printk("************ni->i_hot_page_num==0*********:\n");
					cold_dram[page_start]=1;
					if(page_start<ni->i_cold_page_start){
						ni->i_cold_page_start=page_start;
					}
					ni->i_cold_page_num++;
				}else{
				//����������ҳ
				//printk("************ni->i_hot_page_num  !=0*********:\n");
					for(i=0;i<5;i++){
						if(hot_dram[i]==page_start)
							break;
					}
					if(i==5){
						//��ҳ����������ҳ
						cold_dram[page_start]=1;
						if(page_start<ni->i_cold_page_start){
							ni->i_cold_page_start=page_start;					
						}
						ni->i_cold_page_num++;
					}
					//else����������ݣ�ʲôҲ����
				}
				

			}else{
			//��ҳ��������ҳ
			//printk("************cold_dram[page_start]  !=0*********:\n");
				pud=rdfs_pud_alloc(sb,ino,offset);
				pmd=rdfs_pmd_offset(pud,start_addr);
				pte=rdfs_pte_offset(pmd,start_addr);
				//page_hot=(pte_val(*pte) & 0x0000000000000200)>>9;
				//access=(pte_val(*pte) & 0x0000000000000020)>>5;
				
				if(ni->i_hot_page_num<5){
					
					for(i=0;hot_dram[i]!=NULL&&i<5;i++){}
					hot_dram[i]=page_start;
					ni->i_hot_page_num++;
					set_pte(pte,__pte(pte_val(*pte)|0x0000000000000200));
					set_pte(pte,__pte(pte_val(*pte)&~0x0000000000000020));
					
					if(page_start==ni->i_cold_page_start){
						if(ni->i_cold_page_num>1){
							for(i=page_start+1;cold_dram[i]==NULL;i++){}//==null?????
							ni->i_cold_page_start=i;
							cold_dram[page_start]=0;
							ni->i_cold_page_num--;
							
						}else{
							ni->i_cold_page_start=0;
							cold_dram[page_start]=0;
							ni->i_cold_page_num--;
							
						}	
					}else{
						cold_dram[page_start]=0;
						ni->i_cold_page_num--;

					}
					//cold_dram[page_start]=0;
					//ni->i_cold_page_num--;
					
				}

			}

		}


		
		page_start++;
		offset+=4096;
		start_addr+=4096;
		length-=4096;
	}
	//printk("ino     %d  \n",ino);
	//printk("ni->i_cold_page_start     %d  \n",ni->i_cold_page_start);
	//printk("ni->i_cold_page_num     %d   \n",ni->i_cold_page_num);
	//printk("%s       end     \n\n",__FUNCTION__);
	return 1;
}

ssize_t rdfs_file_read(struct file*filp,char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
	struct address_space *mapping = filp->f_mapping;
	struct inode *inode = mapping->host;
	loff_t offset=*ppos;
	void *start_addr = RDFS_I(inode)->i_virt_addr+*ppos;
	loff_t size=0;
	ssize_t ret=0;
	size=i_size_read(inode);
	if(size<=0)
	{
		return length;
	}
	if(*ppos+length>size)
	length=size-*ppos;


	ret=__copy_to_user(buf,start_addr,length);
	*ppos = *ppos +length;
	int ret1=rdfs_rw_pte_hc(inode,length,offset);
	return length-ret;
}

unsigned long dma_start_addr = 0x210000000;
char __user *current_buf;
unsigned long current_length;
extern int posted_req_cnt;

ssize_t dmfs_file_read(struct file *filp,char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
	struct address_space *mapping = filp->f_mapping;
	struct inode *inode = mapping->host;
	struct rdfs_inode_info * ni_info = RDFS_I(inode);
	struct rdfs_inode* ni = get_rdfs_inode(inode->i_sb,inode->i_ino);
	struct index_search_result *result = kmalloc(sizeof(struct index_search_result) * MAX_BLOCK_NUM,GFP_KERNEL);
	loff_t size=0;
	unsigned long ret=0,ret1=0;
	size=i_size_read(inode);
	if(size<=0)
	{
		return length;
	}
	if(*ppos+length>size)
		length=size-*ppos;
	if(length <= 0)
	{
		return length;
	}
	unsigned long offset = *ppos;
	unsigned long num = dmfs_index_search(ni_info,offset,length,result);
	current_buf = buf;
	current_length = 0;

	int i = 0;
	for(i = 0;i<num;i++)
	{
		spin_lock(&ctx[result[i].slave_id].max_req_id.req_id_lock);
		u64 req_id = ctx[result[i].slave_id].max_req_id.max_req_id;
		ctx[result[i].slave_id].max_req_id.max_req_id = (ctx[result[i].slave_id].max_req_id.max_req_id+1)%64;
		spin_unlock(&ctx[result[i].slave_id].max_req_id.req_id_lock);

		set_bit(req_id,&ctx[result[i].slave_id].max_req_id.lock_word);

		if(i == num - 1)
		{
			current_length = length;
		}
		ret1=ret;
		ret += dmfs_block_rw(req_id,result[i].slave_id,result[i].start_addr,RDFS_READ,result[i].size,(0x210000000 + ret));

		__copy_to_user(buf+ret1,rdfs_va(0x210000000)+ret1,result[i].size);
		ret1=ret;
		printk("-----------------dmfs_file_read dmfs_block_rw\n");
		posted_req_cnt++;
		wait_on_bit_lock(&ctx[result[i].slave_id].max_req_id.lock_word,req_id,TASK_KILLABLE);
	}
	*ppos = *ppos +length;
	return ret;
}

ssize_t dmfs_file_write(struct file *filp,const char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
	struct address_space *mapping = filp->f_mapping;
	struct inode *inode = mapping->host;
	struct rdfs_inode* ni = get_rdfs_inode(inode->i_sb,inode->i_ino);
	
	struct index_search_result *result = kmalloc(sizeof(struct index_search_result) * MAX_BLOCK_NUM,GFP_KERNEL);
	
	struct super_block *sb = inode->i_sb;
	struct rdfs_inode_info *ni_info=RDFS_I(inode);
	long pages_exist = 0, pages_to_alloc = 0,pages_needed = 0; 
	loff_t size=0;
	size = i_size_read(inode);
	unsigned long __offset;
	pte_t* pte;
	struct swap_buffer* swap_buf = &ni_info->swap_buffer;
	ni_info->post = *ppos;
	ni_info->access_length = length;
	size_t retval = 0;

	pages_needed = ((*ppos + length + sb->s_blocksize - 1) >> sb->s_blocksize_bits);
    pages_exist = (size + sb->s_blocksize - 1) >> sb->s_blocksize_bits;
    pages_to_alloc = pages_needed - pages_exist;
	if(pages_to_alloc > 0)
	{
		retval = dmfs_new_block(ni_info,pages_to_alloc * FK_SIZE);
		if (retval < 0){
			rdfs_info("alloc blocks failed!\n");
		}
	}
	unsigned long offset = *ppos;
	unsigned long num = dmfs_index_search(ni_info,offset,length,result);

	current_length = 0;
	retval = __copy_from_user(rdfs_va(dma_start_addr),buf,length);
	unsigned long ret = 0;
	int i = 0;
	for(i = 0;i<num;i++)
	{
		spin_lock(&ctx[result[i].slave_id].max_req_id.req_id_lock);
		u64 req_id = ctx[result[i].slave_id].max_req_id.max_req_id;
		ctx[result[i].slave_id].max_req_id.max_req_id = (ctx[result[i].slave_id].max_req_id.max_req_id+1)%64;
		//printk("%s posted_req_cnt:%d max_req_id:%d\n",__FUNCTION__,posted_req_cnt,ctx[client_idx].max_req_id.max_req_id);
		spin_unlock(&ctx[result[i].slave_id].max_req_id.req_id_lock);

		set_bit(req_id,&ctx[result[i].slave_id].max_req_id.lock_word);

		ret += dmfs_block_rw(req_id,result[i].slave_id,result[i].start_addr,RDFS_WRITE,result[i].size,(0x210000000 + ret));
		//ret += dmfs_block_rw(req_id,result[i].slave_id,result[i].start_addr,RDFS_WRITE,result[i].size,dma_start_addr + ret);
		posted_req_cnt++;
		wait_on_bit_lock(&ctx[result[i].slave_id].max_req_id.lock_word,req_id,TASK_KILLABLE);		
	}
	if(*ppos + length > size){	
		i_size_write(inode, *ppos + length);
	}
	*ppos = *ppos + length;
	return length-retval;
}

ssize_t rdfs_file_write(struct file *filp,const char __user *buf,size_t length,loff_t *ppos)
{
	rdfs_trace();
	struct address_space *mapping = filp->f_mapping;
	struct inode *inode = mapping->host;
	
	void *start_addr = RDFS_I(inode)->i_virt_addr+*ppos;
	loff_t offset=*ppos;
	
	struct super_block *sb = inode->i_sb;
	struct rdfs_inode_info *ni_info=RDFS_I(inode);
	long pages_exist = 0, pages_to_alloc = 0,pages_needed = 0; 
	loff_t size=0;
	size=i_size_read(inode);
	unsigned long __offset;
	pte_t* pte;
	struct swap_buffer* swap_buf=&ni_info->swap_buffer;
	ni_info->post=*ppos;
	ni_info->access_length=length;
	size_t retval=0;
	pages_needed = ((*ppos + length + sb->s_blocksize - 1) >> sb->s_blocksize_bits);
    	pages_exist = (size + sb->s_blocksize - 1) >> sb->s_blocksize_bits;
    	pages_to_alloc = pages_needed - pages_exist;
	if(pages_to_alloc > 0)
	{
		//printk("%s need:%ld,exist:%ld,alloc:%ld,ino:%lx\n",__FUNCTION__,pages_needed,pages_exist,pages_to_alloc,inode->i_ino);
		retval = rdfs_alloc_blocks(inode, pages_to_alloc, 1);
		if (retval){
			rdfs_info("alloc blocks failed!\n");
			//goto out;
		}
	}
	retval = __copy_from_user(start_addr,buf,length);
	if(*ppos + length > size){	
		i_size_write(inode, *ppos + length);
	}
	*ppos = *ppos + length;
	int ret1=rdfs_rw_pte_hc(inode,length,offset);

	return length-retval;
}

ssize_t rdfs_direct_IO(struct kiocb *iocb,struct iov_iter *iter,loff_t offset)
{
	rdfs_trace();
	struct file *file=iocb->ki_filp;
	struct inode*inode=file->f_mapping->host;
	struct super_block*sb=inode->i_sb;
	ssize_t retval=0;
	int hole=0;
	loff_t size;
	void* start_addr=RDFS_I(inode)->i_virt_addr+offset;
	size_t length=iov_iter_count(iter);
	unsigned long pages_exist=0,pages_to_alloc=0,pages_need=0;

	size=i_size_read(inode);
	return iov_iter_count(iter);
}

static int rdfs_check_flags(int flags)
{
	rdfs_trace();
	if(!(flags&O_DIRECT))
		return -EINVAL;

	return 0;
}
const struct file_operations rdfs_file_operations =
{
	.llseek = generic_file_llseek,
	.read_iter = generic_file_read_iter,
	.write_iter = generic_file_write_iter,
	.read = dmfs_file_read,
	.write = dmfs_file_write,
	.mmap = generic_file_mmap,
	.open = rdfs_open_file,
	.nvrelease = rdfs_release_file,
	.fsync = noop_fsync,
	.check_flags = rdfs_check_flags,
};

#ifdef CONFIG_RDFS_XIP
const struct file_operations rdfs_xip_file_operations = 
{
/*	.llseek		= generic_file_llseek,
	.read		= xip_file_read,
	.write		= xip_file_write,
	.mmap		= xip_file_mmap,
	.open		= generic_file_open,
	.fsync		= noop_fsync,*/
};
#endif


const struct inode_operations rdfs_file_inode_operations = 
{
#ifdef CONFIG_RDFS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= rdfs_listxattr,
	.removexattr	= generic_removexattr,
#endif
	.setattr	= rdfs_notify_change,
	.get_acl	= rdfs_get_acl,
};

