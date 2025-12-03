obj-m := gs_execmon.o

KDIR ?= $(KERNEL_BUILD_DIR)
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

