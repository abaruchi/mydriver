ifneq ($(KERNELRELEASE),)
	obj-m := get_info.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

clean:
	rm -rf *.o *~ *# *.symvers core .depend .*.cmd *.ko *.mod.c .tmp_versions

module:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif
