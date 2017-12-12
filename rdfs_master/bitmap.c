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
 
 int rdfs_init_slave_memory_bitmap_list(struct slave_info * s)
 {
     int block_nums = s->free_block_nums;
     int array_nums = block_nums / BITMAP_ARRAY_BLOCK_NUMS + 1;
     
     int rest_block_nums = block_nums % BITMAP_ARRAY_BLOCK_NUMS;
     s->bitmap_array_nums = array_nums;
     struct mem_bitmap ** bitmap = (struct mem_bitmap*)kmalloc(sizeof(struct mem_bitmap) * array_nums);
     int i,j;
     for(i=0;i<array_nums;i++)
     {
         spin_lock_init(&bitmap[i]->map_lock);
         bitmap[i]->flag = 1;
         memset(bitmap[i]->map,0,sizeof(unsigned long)*BITMAP_ARRAY_LEN);
         if(i == array_nums -1 && rest_block_nums)
         {
             int tmp = rest_block_nums / 64;
             for(j=0;j<tmp;j++)
                 bitmap[i]->map[j] = BITMAP_MASK;
             int r = rest_block_nums % 64;
             if(r)
             {
                 unsigned long t = 0;
                 for(k=0;k<r;k++)
                     t |= (1<<r);
                 bitmap[i]->map[j+1] = t;
             }	
         }
         else
         {
             for(j=0;j<BITMAP_ARRAY_LEN;j++)
                 bitmap[i]->map[j] = BITMAP_MASK;
         }
     }
     s->bitmap = bitmap;
     return 0;
 }
 int rdfs_free_slave_memory_bitmap_list(struct slave_info *s)
 {
     s->free_block_nums = 0;
     kfree(s->bitmap);
     return 0;
 }
 //1 means free ,0 means alloced
 int rdfs_new_block(int *s_id,int *block_num)
 {
     int id = rand() % slave_id;
 
     struct mem_bitmap **bitmap = slave_infos[id].bitmap;
 
     int array_id = rand() % slave_infos[id].bitmap_array_nums;
     while(!bitmap[array_id]->flag)
         array_id = rand() % slave_infos[id].bitmap_array_nums;
     int i;
     unsigned long bits;
     unsigned long res;
     spin_lock(&bitmap[array_id]->map_lock);
     for(i=0;i<BITMAP_ARRAY_LEN;i++)
     {
         bits = bitmap[array_id]->map[i];
         res = bits & (~bits + 1);
         if(res == 0)continue;
         bitmap[array_id]->map[i] ^= res;
     }
     *s_id = id;
     int cnt = 0;
     while(res!=1)
     {
         res = res >> 1;
         cnt++;
     }
     *block_num = array_id * BITMAP_ARRAY_BLOCK_NUMS + i * 64 + cnt;
     spin_unlock(&bitmap[array_id]->map_lock);
     return 0;
 }
 int rdfs_free_block(int s_id,int block_num)
 {
     int array_id = block_num / BITMAP_ARRAY_BLOCK_NUMS;
     int tmp = block_num % BITMAP_ARRAY_BLOCK_NUMS ;
     int a_id = tmp / 64;
     int cnt = tmp % 64;
     struct mem_bitmap **bitmap = slave_infos[s_id].bitmap;
     bitmap[array_id]->map[a_id]
 }
 