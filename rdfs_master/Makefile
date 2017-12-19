obj-m:= rdfs.o 
rdfs-objs = acl.o  debug.o pgtable.o  balloc.o   namei.o \
symlink.o file.o  nvmalloc.o network.o master_service.o \
 dir.o inode.o  server.o memory.o device.o rdfs_rdma.o \
 main.o tool.o super.o
.PHONY:load unload
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
load:
	sudo insmod rdfs.ko
unload:
	sudo rmmod -f rdfs
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
