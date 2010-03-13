
# Makefile
# Declare Name of kernel module
KMOD = test

# Enumerate Source files for kernel module
SRCS = test.c
SRCS += device_if.h bus_if.h

i:
	sudo kldload ./test.ko
r:
	sudo kldunload ./test.ko


.include <bsd.kmod.mk>

