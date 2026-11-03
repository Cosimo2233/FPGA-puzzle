obj-m += ec26e_hdmi_fb.o
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
MODULE_BUILD ?= $(abspath $(CURDIR)/../../../build/generated/linux/hdmi-fb)
# Kbuild 的大小写要求只在可删除的生成目录中恢复。
all:
	mkdir -p $(MODULE_BUILD)
	cp makefile $(MODULE_BUILD)/Makefile
	cp ec26e_hdmi_fb.c $(MODULE_BUILD)/
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_BUILD) modules
clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_BUILD) clean
