obj-m:= slave.o
slave-objs = event.o memory.o  rdfs_sock.o rdfs.o main.o
.PHONY:load unload
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
load:
	sudo insmod slave.ko
unload:
	sudo rmmod -f slave
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

