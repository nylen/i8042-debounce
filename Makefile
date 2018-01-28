modname := i8042_debounce
obj-m := $(modname).o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
KMISC := /lib/modules/$(KVERSION)/misc
PWD := "$$(pwd)"

ifdef DEBUG
CFLAGS_$(obj-m) := -DDEBUG
endif

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) O=$(PWD) -C $(KDIR) M=$(PWD) clean

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
