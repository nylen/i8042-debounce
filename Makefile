modname := i8042_debounce
obj-m := $(modname).o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
KMISC := /lib/modules/$(KVERSION)/misc

ifdef DEBUG
CFLAGS_$(obj-m) := -DDEBUG
endif

STANDARD_MSEC := 40
MULTIKEY_MSEC := 55
INTERKEY_MSEC :=  5
MESSAGE_MSEC := 100

CFLAGS_$(obj-m) += -DSTANDARD_MSEC=$(STANDARD_MSEC)
CFLAGS_$(obj-m) += -DMULTIKEY_MSEC=$(MULTIKEY_MSEC)
CFLAGS_$(obj-m) += -DINTERKEY_MSEC=$(INTERKEY_MSEC)
CFLAGS_$(obj-m) += -DMESSAGE_MSEC=$(MESSAGE_MSEC)

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
