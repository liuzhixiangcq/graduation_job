obj-m:= rdfs.o 
rdfs-objs = acl.o  debug.o pgtable.o rdfs_swap.o balloc.o   namei.o \
symlink.o file.o  nvmalloc.o rdfs_page_fault.o \
 dir.o inode.o rdfs_inode_lru_op.o server.o memory.o \
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
