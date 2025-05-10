# See example Makefile from scull project
# Comment/uncomment the following line to disable/enable debugging
#DEBUG = y

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)
EXTRA_CFLAGS += -I$(PWD)/char-driver/include
EXTRA_CFLAGS += -I$(PWD)/circular-buffer/include

ifneq ($(KERNELRELEASE),)
# call from kernel build system
obj-m := aesdchar.o
aesdchar-y := main.o \
              char-driver/src/aesd-char-device.o \
              char-driver/src/aesd-char-fileops.o \
              char-driver/src/aesd-char-buffer.o \
              circular-buffer/src/aesd-circular-buffer-add.o \
              circular-buffer/src/aesd-circular-buffer-remove.o \
              circular-buffer/src/aesd-circular-buffer-init.o \
              circular-buffer/src/aesd-circular-buffer-find.o
else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions \
	       char-driver/src/*.o char-driver/src/.*.cmd \
	       circular-buffer/src/*.o circular-buffer/src/.*.cmd \
	       Module.symvers modules.order
