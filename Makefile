
export FW_VER := 1.0

PROJ := GW_TestBoard
VER := v$(FW_VER)

SUBDIRS += src

.PHONY: all clean ocd serial gotek

ifneq ($(RULES_MK),y)

.DEFAULT_GOAL := gotek
export ROOT := $(CURDIR)

all:
	$(MAKE) -f $(ROOT)/Rules.mk all

clean:
	rm -f *.hex *.dfu *.html
	$(MAKE) -f $(ROOT)/Rules.mk $@

gotek: all
	cp src/$(PROJ).hex $(PROJ)-$(VER).hex
	cp src/$(PROJ).dfu $(PROJ)-$(VER).dfu

dist:
	rm -rf gw_testboard-*
	mkdir -p gw_testboard-$(VER)
	$(MAKE) clean
	$(MAKE) gotek
	cp -a $(PROJ)-$(VER).hex gw_testboard-$(VER)/
	cp -a $(PROJ)-$(VER).dfu gw_testboard-$(VER)/
	$(MAKE) clean
	cp -a COPYING gw_testboard-$(VER)/
	zip -r gw_testboard-$(VER).zip gw_testboard-$(VER)

mrproper: clean
	rm -rf gw_testboard-*

else

all:
	$(MAKE) -C src -f $(ROOT)/Rules.mk $(PROJ).elf $(PROJ).bin $(PROJ).hex $(PROJ).dfu

endif

BAUD=115200
DEV=/dev/ttyUSB0

ocd: gotek
	python3 scripts/openocd/flash.py `pwd`/$(PROJ)-$(VER).hex

serial:
	sudo miniterm.py $(DEV) 3000000
