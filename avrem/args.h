#ifndef _ARGS_H_
#define _ARGS_H_

#include <stdint.h>
#include "config.h"

#define USART_REDIRECT_NONE			0
#define USART_REDIRECT_STDIO		1
#define USART_REDIRECT_TCP			2

struct avr_config_s {
	char* file_elf;
	char* file_hex;
	char* file_eeprom;
	char* mcu;
	uint32_t osc;
	uint8_t usart_redirect[USART_MAX_COUNT];
	uint16_t usart_tcp_port[USART_MAX_COUNT];
	uint16_t extport;
	uint8_t showclock;
	uint8_t pause;
	uint8_t pause_ext;
	int log_level;
};

int args_parse(struct avr_config_s* avr_config, int argc, char* argv[]);
void args_showHelp();

int init_avr(struct avr_core_s* core, struct avr_config_s* avr_config);

#endif
