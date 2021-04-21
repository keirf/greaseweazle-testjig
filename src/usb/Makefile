OBJS += usb_bsp.o
OBJS += usbh_glue.o

SUBDIRS += stm32_usbh

usb%.o: CFLAGS += -Istm32_usbh/inc/ -include usbh_conf.h
