obj-m += ec26e_accel.o
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
MODULE_BUILD ?= $(abspath $(CURDIR)/../../../build/generated/linux/accel)
# 交付文件保持小写，在生成区恢复 Linux Kbuild 需要的 Makefile。
all:
	mkdir -p $(MODULE_BUILD)
	cp makefile $(MODULE_BUILD)/Makefile
	cp ec26e_accel.c $(MODULE_BUILD)/
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_BUILD) modules
