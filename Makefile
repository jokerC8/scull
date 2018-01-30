KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
obj-m := scull.o
modules:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
.PHONY:clean
clean:
	@rm -rf *.o *.mod.c *.order *.symvers *.ko .scull* .tmp_versions
