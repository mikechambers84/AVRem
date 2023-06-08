#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifdef __WATCOMC__
	#define _putch putch
	#define _getch getch
	#define _kbhit kbhit
#endif

#define STR_TITLE "AVRem"
#define STR_VER "v0.1.0"

//Enabling accurate peripheral clocks causes peripherals clocking to be called after every CPU instruction.
//This will allow for the most accurate timing, but is much slower and may cause performance issues older systems.
#define ACCURATE_PERIPHERAL_CLOCKS

//Enables throttling of the emulated MCU to the specified speed. If not defined, it'll run as fast as possible.
//You probably want this, but it could cause AVRem to not compile on certain (uncommon) platforms.
#define THROTTLING_ENABLED

//If enabled, the thread still uses CPU time while throttling MCU speed. If disabled, it sleeps. Sleeping makes
//it slightly less accurate, but will consume a lot less system resources. In my limited testing, the speed error
//rate is only 0.2% with this turned off, so there aren't many situations where this would be necessary.
//#define ACCURATE_THROTTLING

#ifdef _WIN32
//TCP is only supported on Windows right now. Still need to add support for other OSes.
#define USE_TCP
#endif

#define USART_MAX_COUNT			5

#endif
