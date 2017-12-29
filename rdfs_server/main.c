/* @author page
 * @date 2017/12/26
 */

 #include <stdio.h>
 #include <unistd.h>

 #include "rdfs_config.h"

 int main()
 {
     int ret = 0;
     ret = rdfs_init_network(MASTER_IP,CLIENT_USERSPACE_REQUEST_PORT);
}