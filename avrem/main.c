#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include "loglevel.h"
#include "tcpconsole.h"
#include "config.h"
#include "timing.h"
#include "args.h"
#include "intelhex.h"
#include "elf.h"
#include "timer16.h"
#include "usart.h"
#include "adc.h"
#include "periph.h"
#include "eeprom.h"
#include "avrcore.h"
#include "module.h"

#ifdef THROTTLING_ENABLED
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#endif

extern struct usart_s usart[USART_MAX_COUNT];
extern struct adc_s adc;

extern volatile int ext_start;

struct avr_core_s avr;
struct avr_config_s avr_config;

uint64_t total_executed = 0;
uint32_t build_id = 0;

int timer_wait = 0;

void create_build_id() {
	int i, j;
	uint32_t byte, crc, mask;
#ifdef _WIN32
	FILE* self;
	char ExeFileName[MAX_PATH];
	uint8_t* tmp;
	size_t szSelf;

	if (!GetModuleFileNameA(NULL, ExeFileName, MAX_PATH)) {
		printf("ERROR: Unable to get own file name!\n");
		return;
	}

	self = fopen(ExeFileName, "rb");
	if (self == NULL) {
		printf("ERROR: Unable to open self!\n");
		return;
	}
	fseek(self, 0, SEEK_END);
	szSelf = ftell(self);
	fseek(self, 0, SEEK_SET);
	tmp = malloc(szSelf);
	if (tmp == NULL) {
		printf("ERROR: Unable to allocate memory to read self!");
		return;
	}
	fread(tmp, 1, szSelf, self);
	fclose(self);

	crc = 0xFFFFFFFFLU;
	for (i=0; i<szSelf; i++) {
		byte = tmp[i];
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {
			mask = -((int32_t)(crc & 1));
			crc = (crc >> 1) ^ (0xEDB88320LU & mask);
		}
	}
	build_id = ~crc;
#else
	//TODO: handle other platforms properly later
	uint8_t tmp[32];
	sprintf(tmp, __DATE__ " " __TIME__);
	i = 0;
	crc = 0xFFFFFFFFLU;
	while (tmp[i] != 0) {
		byte = tmp[i];
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--) {
			mask = -((int32_t)(crc & 1));
			crc = (crc >> 1) ^ (0xEDB88320LU & mask);
		}
		i = i + 1;
	}
	build_id = ~crc;
#endif
}

void timing_callback(void* dummy) {
	timer_wait = 0;
}

int main(int argc, char* argv[]) {
	int cycle_overflow = 0, i, dummy;
	int clocks_per_loop;
	uint64_t start_time, end_time;

	create_build_id();

	printl(LOG_NONE, STR_TITLE " " STR_VER " %08x (c)2023 Mike Chambers (Build date: " __DATE__ " " __TIME__ ")\n", build_id);
	printl(LOG_NONE, "[An 8-bit AVR microcontroller emulator]\n\n");


	avr_config.file_eeprom = avr_config.file_elf = avr_config.file_hex = avr_config.mcu = NULL;
	avr_config.osc = 0;
	avr_config.log_level = LOG_ERROR;
	if (args_parse(&avr_config, argc, argv)) {
		return -1;
	}
	log_setLevel(avr_config.log_level);

	if (avr_config.mcu == NULL) {
		printf("No MCU has been specified, use -h for help.\n");
		return -1;
	}

	if ((avr_config.file_elf == NULL) && (avr_config.file_hex == NULL)) {
		printf("No firmware has been specified, use -h for help.\n");
		return -1;
	}

	if (init_avr(&avr, &avr_config)) {
		return -1;
	}

	if (avr_config.file_hex != NULL) {
		printl(LOG_INFO, "Loading firmware: %s\n", avr_config.file_hex);
		if (intelhex_load(&avr, avr_config.file_hex)) {
			printl(LOG_ERROR, "Error loading HEX.\n");
			return -1;
		}
	}
	else {
		printl(LOG_INFO, "Loading firmware: %s\n", avr_config.file_elf);
		if (elf_load(&avr, avr_config.file_elf)) {
			printl(LOG_ERROR, "Error loading ELF.\n");
			return -1;
		}
	}

	if (avr_config.file_eeprom != NULL) {
		if (eeprom_init(&avr, avr_config.file_eeprom)) {
			printl(LOG_ERROR, "ERROR: Unable to open or create EEPROM file.\n");
		}
		printl(LOG_INFO, "Opened EEPROM file: %s\n", avr_config.file_eeprom);
	}
	else {
		printl(LOG_INFO, "No EEPROM file specified, data will be lost after AVRem shutdown.\n");
	}

	for (i = 0; i < USART_MAX_COUNT; i++) {
		if (avr_config.usart_redirect[i] == USART_REDIRECT_STDIO) {
			printl(LOG_INFO, "Configuring USART%d for stdio redirection\n", i);
		}
		else if (avr_config.usart_redirect[i] == USART_REDIRECT_TCP) {
			printl(LOG_INFO, "Configuring USART%d for TCP redirection on port %u\n", i, avr_config.usart_tcp_port[i]);
			tcpconsole_init(&usart[i], avr_config.usart_tcp_port[i]);
		}
	}

	if (avr_config.extport) {
		printl(LOG_INFO, "Configuring external interface on TCP port %u\n", avr_config.extport);
		if (tcpconsole_init(NULL, avr_config.extport)) {
			printl(LOG_ERROR, "Error initializing external interface.\n");
			return -1;
		}
		avr.external = 1;
	}

	module_init(&avr);
	avr_reset(&avr);

	if (avr_config.pause || avr_config.pause_ext) {
		printf("Press a key %sto start emulator...", avr_config.pause_ext ? "or connect external interface " : ""); fflush(stdout);
		if (avr_config.pause_ext && avr.external) {
			while (!_kbhit() && !ext_start) {
				tcpconsole_dorecv(NULL);
			}
		}
		if (!ext_start) _getch();
		printf("\n\n");
	}

#ifdef THROTTLING_ENABLED
	timing_init();
	if (avr_config.osc) {
		clocks_per_loop = avr_config.osc / 1000;
		timing_addTimer(timing_callback, NULL, 1000, TIMING_ENABLED);
	}
	else {
		clocks_per_loop = 500;
	}
	start_time = timing_getCur();
	//printf("clocks per loop %d\n", clocks_per_loop);
#else
	clocks_per_loop = 1024;
#endif

	while (1) {
		cycle_overflow = avr_execute(&avr, clocks_per_loop + cycle_overflow);
#ifndef ACCURATE_PERIPHERAL_CLOCKS
		periph_clock(&avr, clocks_per_loop + cycle_overflow);
		module_clock(clocks_per_loop + cycle_overflow);
#endif
		total_executed += (uint64_t)(clocks_per_loop + cycle_overflow);
		if (cycle_overflow == 0x80) break;

		for (i = 0; i < USART_MAX_COUNT; i++) {
			if (avr_config.usart_redirect[i] == USART_REDIRECT_TCP) {
				tcpconsole_dorecv(&usart[i]);
			}
		}

		if (avr.external) {
			tcpconsole_dorecv(NULL);
		}

		if (_kbhit()) {
			uint8_t cc;
			cc = (uint8_t)_getch();
			if (cc == 27) break; //break loop on escape press
			
			for (i = 0; i < USART_MAX_COUNT; i++) {
				if (avr_config.usart_redirect[i] == USART_REDIRECT_STDIO) {
					usart_rx(&usart[i], cc);
				}
			}
		}

#ifdef THROTTLING_ENABLED
		if (avr_config.osc) {
			timer_wait = 1;
			while (1) {
				timing_loop();
				if (timer_wait == 0) break;
#ifndef ACCURATE_THROTTLING
#ifdef _WIN32
				Sleep(1);
#else
				sleep(0.01);
#endif
#endif

			}
		}
#endif
	}
	printl(LOG_INFO, "\n");
#ifdef THROTTLING_ENABLED
	end_time = timing_getCur();
	printl(LOG_INFO, "%llu instructions executed in %f seconds\n", total_executed,
		(double)(end_time - start_time) / (double)timing_getFreq()
	);
	printl(LOG_INFO, "Average CPU frequency: %llu Hz\n",
		(uint64_t)((double)total_executed / ((double)(end_time - start_time) / (double)timing_getFreq()))
	);
#endif
	printl(LOG_INFO, "Total executed instructions: %lu\n", total_executed);

	printf("%c[37;40m", 27); fflush(stdout);
	return 0;
}
