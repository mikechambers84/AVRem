#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "loglevel.h"
#include "args.h"
#include "module.h"

void list_avr_models();

int args_isMatch(char* s1, char* s2) {
	int i = 0, match = 1;

	while (1) {
		char c1, c2;
		c1 = s1[i];
		c2 = s2[i++];
		if ((c1 >= 'A') && (c1 <= 'Z')) {
			c1 -= 'A' - 'a';
		}
		if ((c2 >= 'A') && (c2 <= 'Z')) {
			c2 -= 'A' - 'a';
		}
		if (c1 != c2) {
			match = 0;
			break;
		}
		if (!c1 || !c2) {
			break;
		}
	}

	return match;
}

void args_showHelp() {
	printf("Command line parameters:\n");

	printf("\nEmulator options:\n");
	printf("  -mcu <model>               Specify MCU model. Use \"-mcu list\" to show available options.\n");
	printf("  -elf <input file>          Specify ELF file firmware.\n");
	printf("  -hex <input file>          Specify Intel Hex file firmware.\n");
	printf("  -eeprom <input file>       Specify EEPROM file. If it doesn't exist, it will be created.\n");
	printf("  -osc <hz>                  Specify external oscillator clock speed to emulate in Hz. Default is 8000000.\n");

	printf("\nWatchdog options:\n");
	printf("  -exitwdt                   Treat WDT terminal count as an error condition and exit emulator.\n");
	printf("  -nowdt                     Disable WDT counter entirely.\n");

	printf("\nUSART options:\n");
#ifdef USE_TCP
	printf("  -usart <n> <mode> [port]   Choose USART <n> redirection mode. Valid modes are stdio and tcp.\n");
	printf("                             If tcp, then listening port must also be specified.\n");

	printf("\nExternal interface:\n");
	printf("   -ext <port>               Allow connection from external program on TCP port.\n");
#else
	printf("  -usart <n> <mode>          Choose USART <n> redirection mode. The only valid mode is stdio.\n");
	printf("                             Recompile AVRem with USE_TCP to allow TCP socket redirection.\n");
#endif
	printf("   -mod <file>               Load a DLL as an AVRem external module.\n");

	printf("\nMiscellaneous options:\n");
	//printf("  -showclock                 Display actual clock speed.\n");
	printf("  -pause                     Pause and wait for key press before starting emulation.\n");
	printf("  -pauseext                  Pause and wait for external interface connection before starting emulation.\n");
	printf("  -log <level>               Logging levels: silent, error, info, detail, detail2.\n");
	printf("                             Default level is error.\n");
	printf("  -h                         Show this help screen.\n");

}

void args_listcpu() {
	list_avr_models();
	exit(0);
}

int args_parse(struct avr_config_s* avr_config, int argc, char* argv[]) {
	int i, choseproduct = 0;

	if (argc < 2) {
		printf("Use -h for help.\n");
		return -1;
	}

	for (i = 1; i < argc; i++) {
		if (args_isMatch(argv[i], "-h")) {
			args_showHelp();
			return -1;
		}
		else if (args_isMatch(argv[i], "-elf")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -elf. Use -h for help.\n");
				return -1;
			}
			avr_config->file_elf = argv[++i];
		}
		else if (args_isMatch(argv[i], "-hex")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -hex. Use -h for help.\n");
				return -1;
			}
			avr_config->file_hex = argv[++i];
		}
		else if (args_isMatch(argv[i], "-eeprom")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -eeprom. Use -h for help.\n");
				return -1;
			}
			avr_config->file_eeprom = argv[++i];
		}
		else if (args_isMatch(argv[i], "-osc")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -osc. Use -h for help.\n");
				return -1;
			}
			avr_config->osc = atol(argv[++i]);
			if (avr_config->osc < 1000) {
				printf("Invalid oscilator speed. Must be at least 1000 Hz.\n");
				return -1;
			}
		}
		else if (args_isMatch(argv[i], "-mcu")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -mcu. Use -h for help.\n");
				return -1;
			}
			avr_config->mcu = argv[++i];
			if (args_isMatch(argv[i], "list")) {
				args_listcpu();
			}
		}
		else if (args_isMatch(argv[i], "-usart")) {
			int usart_num;
			if ((i + 1) == argc) {
				printf("Parameter(s) required for -usart. Use -h for help.\n");
				return -1;
			}

			usart_num = atoi(argv[++i]);
			if ((usart_num < 0) || (usart_num >= USART_MAX_COUNT)) {
				printf("Invalid USART number: %s\n", argv[i]);
				return -1;
			}

			i++;
			if (args_isMatch(argv[i], "stdio")) {
				avr_config->usart_redirect[usart_num] = USART_REDIRECT_STDIO;
			}
#ifdef USE_TCP
			else if (args_isMatch(argv[i], "tcp")) {
				if ((i + 1) == argc) {
					printf("Port number required for -usart %d tcp. Use -h for help.\n", usart_num);
					return -1;
				}
				avr_config->usart_tcp_port[usart_num] = (uint16_t)atol(argv[++i]);
				avr_config->usart_redirect[usart_num] = USART_REDIRECT_TCP;
			}
#endif
			else {
				printf("Invalid USART %d redirect option: %s\n", usart_num, argv[i]);
				return -1;
			}
		}
#ifdef USE_TCP
		else if (args_isMatch(argv[i], "-ext")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -ext. Use -h for help.\n");
				return -1;
			}

			avr_config->extport = (uint16_t)atol(argv[++i]);
		}
#endif
		else if (args_isMatch(argv[i], "-mod")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -mod. Use -h for help.\n");
				return -1;
			}

			module_load(argv[++i]);
		}
		else if (args_isMatch(argv[i], "-log")) {
			if ((i + 1) == argc) {
				printf("Parameter required for -log. Use -h for help.\n");
				return -1;
			}
			i++;
			if (args_isMatch(argv[i], "silent")) avr_config->log_level = LOG_SILENT;
			else if (args_isMatch(argv[i], "error")) avr_config->log_level = LOG_ERROR;
			else if (args_isMatch(argv[i], "info")) avr_config->log_level = LOG_INFO;
			else if (args_isMatch(argv[i], "detail")) avr_config->log_level = LOG_DETAIL;
			else if (args_isMatch(argv[i], "detail2")) avr_config->log_level = LOG_DETAIL2;
			else {
				printf("Invalid log level: %s\nUse -h for help.\n", argv[i]);
				return -1;
			}
		}
		else if (args_isMatch(argv[i], "-showclock")) {
			avr_config->showclock = 1;
		}
		else if (args_isMatch(argv[i], "-pause")) {
			avr_config->pause = 1;
		}
		else if (args_isMatch(argv[i], "-pauseext")) {
			avr_config->pause_ext = 1;
		}
		else {
			printf("Unrecognized parameter: %s\n\nUse -h for help.\n", argv[i]);
			return -1;
		}
	}

	return 0;
}
