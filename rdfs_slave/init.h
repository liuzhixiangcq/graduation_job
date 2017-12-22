/* @author page
 * @date 2017/11/30
 * slave configure information
 */

#ifndef _INIT_H_
#define _INIT_H_

#define MASTER_IP "192.168.0.2"
#define SLAVE_IP "192.168.0.3"

// master listen on port 18515 to wait slave node register
#define MASTER_REGISTER_PORT 18515

// slave listen on port 18518 to wait client node exchange information
#define SLAVE_REGISTER_PORT 18518

// slave infiniband network card port
#define SLAVE_INFINIBAND_CARD_PORT 1

// slave mapping phy addr
#define SLAVE_MAPPING_PHY_ADDR 0X200000000

//slave mapping phy size 4G
#define SLAVE_MAPPING_PHY_SIZE 0X100000000

#endif
