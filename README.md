# Greaseweazle Test Jig

This repository contains the Greaseweazle Test Jig firmware. There are
no binary releases: You will need to compile the firmware yourself.

### Test Jig hardware

If you want to build a Test Jig, you can find the design files
[here][gwdesign].

### Building the firmware

The following instructions apply specifically to **Ubuntu**. They should
apply straightforwardly to other Debian variants.

To install the prerequisites:
```
$ sudo apt install git gcc-arm-none-eabi stm32flash zip
```

To build the Greaseweazle firmware, producing a distribution folder and zip
archive:
```
$ git clone https://github.com/keirf/greaseweazle-testjig.git
$ cd greaseweazle-testjig
$ make dist
```

### Flashing the firmware

The easiest method is to use ArteryISP software on Windows. Follow the
[FlashFloppy guide][ffflash] but use the test jig HEX file you built
earlier. To enter USB DFU mode, you will need two jumpers/straps:
- Across **BOOT0** and **3.3V** on the 5x2 pin header
- Across pins **32** and **33** (labelled **PROG**) on the floppy header

### Redistribution

Most code is public domain; the rest is MIT/BSD or Open Source friendly
(see the [COPYING](COPYING) file).

The source code is freely redistributable in any form. Because this firmware
includes third-party code, you should include or link to the
[COPYING](COPYING) file in any redistribution.

[gwdesign]: https://github.com/keirf/greaseweazle/wiki/Design-Files
[ffflash]: https://github.com/keirf/flashfloppy/wiki/Firmware-Programming