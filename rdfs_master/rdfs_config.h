/* @author page
 * @date 2017/12/5
 * rdfs_master/rdfs_config.h
 */

 #ifndef _RDFS_CONFIG_H
 #define _RDFS_CONFIG_H

 #define MASTER_IP "192.168.0.2"
 
 #define SLAVE_REGISTER_PORT 18515
 #define CLIENT_REQUEST_PORT 18514

 #define INFINIBAND_DEV_PORT 1
 #define INFINIBAND_MAX_SR_WR_NUMS 10
 #define INFINIBAND_MAX_SR_SGE_NUMS 1

 #define BITMAP_ARRAY_LEN  10
 #define BITMAP_ARRAY_BLOCK_NUMS (64*BITMAP_ARRAY_SIZE)
 
 #define RDFS_BLOCK_SHIFT   (22) 
 #define RDFS_BLOCK_SIZE    (1<<22) //4M 
 #define RDFS_BLOCK_OFFSET_MASK (0X3FFFFF)
 #define BITMAP_MASK        0XFFFFFFFFFFFFFFFF
 #define MAX_SLAVE_NUMS 10

 #define RDFS_SLAVE_BLOCK_SIZE (1<<22)
 #endif