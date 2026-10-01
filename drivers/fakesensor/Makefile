obj-m += fakesensor.o

KDIR ?= ../../buildroot/output/build/linux-6.18.7
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
