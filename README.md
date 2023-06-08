# AVRem

**AVRem** is an 8-bit AVR microcontroller emulator written in C.

This software is **not** release-ready, however it supports a good selection of the more popular AVR MCUs and several on-chip peripherals already. So far, it has only been tested on Windows x64 and ARM, but full support for Linux and MacOS is planned. It will probably not compile as-is on them now.

There is some preliminary support for loading custom-made DLLs as external modules, but it needs to be improved and have more features added. The idea is that this can be used to create and attach other emulated devices, such as buttons, LCD displays, or even something like an Ethernet adapter without having to modify the core AVRem software.

Some of the more well-tested MCUs so far include ATmega8, ATmega32, ATmega64, ATmega640, ATmega1280 and ATmega2560. Some of the other MCUs listed as "supported" in the software do not actually set up all the peripherals yet.

There is a lot of work left to do, but I wanted to get something on GitHub now.


## Features
- Load firmware from HEX or ELF files
- EEPROM file support
- Preliminary, incomplete support for adding external modules via DLL
- USART IO redirection through either stdio or TCP socket (currently server-only)
- Nearly complete AVR instruction set support (Missing BREAK, SPM, ESPM)

## Supported AVR peripherals
- USART
- 16-bit timers (Modes 0, 1, and 4)
- EEPROM
- ADC (Though there is no great way to use it yet)

## Still to do
- Allow use of real host serial ports for USART IO
- Allow outgoing TCP connections for USART IO, not only incoming
- Fuses
- Watchdog
- AVR bootloader support (It will always start execution at address zero now)
- 8-bit timers, SPI and all other on-chip peripherals
- Make external module interface more robust
- Some way to easily debug AVR code
- Verify all instructions work accurately
- Verify timing on instructions (I know some are slightly off depending on MCU model)
- Optimize code and improve speed, when compiled in accurate peripheral clocking mode, it only can emulate an AVR at roughly 12 to 15 MHz on a modern CPU.
- Expand this readme
- Lots and lots of other stuff
