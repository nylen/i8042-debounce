modname := i8042_debounce
obj-m := $(modname).o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
KMISC := /lib/modules/$(KVERSION)/misc

ifdef DEBUG
CFLAGS_$(obj-m) := -DDEBUG
endif

MULTI_THRESHOLD := 90
SINGLE_THRESHOLD := 75
MSG_THRESHOLD := 100

CFLAGS_$(obj-m) += -DMULTI_THRESHOLD=$(MULTI_THRESHOLD)
CFLAGS_$(obj-m) += -DSINGLE_THRESHOLD=$(SINGLE_THRESHOLD)
CFLAGS_$(obj-m) += -DMSG_THRESHOLD=$(MSG_THRESHOLD)

default:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) O=$(CURDIR) -C $(KDIR) M=$(CURDIR) clean

load:
	-rmmod $(modname)
	insmod $(modname).ko

install:
	mkdir -p $(KMISC)/$(modname)
	install -m 0755 -o root -g root $(modname).ko $(KMISC)/$(modname)
	depmod -a

uninstall:
	rm $(KMISC)/$(modname)/$(modname).ko
	rmdir $(KMISC)/$(modname)
	rmdir $(KMISC)
	depmod -a
