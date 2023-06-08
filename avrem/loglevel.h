#ifndef _LOGLEVEL_H_
#define _LOGLEVEL_H_

#include <stdint.h>

/*
	Log levels:

	-1 - Completely silent besides USART stdio redirection
	0 - No logging
	1 - Errors
	2 - Errors, info
	3 - Errors, info, detailed debugging
	4 - Errors, info, detailed debugging, super detailed debugging
*/

#define LOG_SILENT		-1
#define LOG_NONE		0
#define LOG_ERROR		1
#define LOG_INFO		2
#define LOG_DETAIL		3
#define LOG_DETAIL2		4

void printl(uint8_t level, char* format, ...);
void log_setLevel(uint8_t level);
void log_init();

#endif
