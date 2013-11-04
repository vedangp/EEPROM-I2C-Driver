ifneq ($(KERNELRELEASE),) 
   obj-m := i2c-flash.o
else 

KERNELDIR ?= ~/Kernel_Source_Trees/Angstrom_Linux/kernel/kernel/ 

PWD := $(shell pwd)

default: 
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-angstrom-linux-gnueabi- -C $(KERNELDIR) M=$(PWD) modules  
	arm-angstrom-linux-gnueabi-gcc i2c-flash_test.c -o i2c-flash-test
endif 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm squeue_test