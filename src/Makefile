OBJS += build_info.o
OBJS += crc.o
OBJS += vectors.o
OBJS += main.o
OBJS += string.o
OBJS += stm32f10x.o
OBJS += time.o
OBJS += timer.o
OBJS += util.o
OBJS += led_7seg.o

OBJS-$(debug) += console.o

SUBDIRS += usb

.PHONY: build_info.c
build_info.o: CFLAGS += -DFW_VER="\"$(FW_VER)\""
