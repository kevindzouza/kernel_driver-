obj-m += vsensor.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod vsensor.ko

unload:
	sudo rmmod vsensor

reload: unload load

logs:
	sudo dmesg -wH
