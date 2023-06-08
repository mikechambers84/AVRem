#ifndef _MODULE_H_
#define _MODULE_H_

#define MODULES_MAX_COUNT		64

#include <stdint.h>
#include <Windows.h>
#include "avrcore.h"

struct module_s {
	char* name;
	HINSTANCE instance;
	int (*init)(void* module); //pointer to module struct for module's init to fill in name
	void (*uninit)(void);
	int (*io_write)(uint32_t addr, uint8_t* val); //return 0 on success
	int (*io_read)(uint32_t addr, uint8_t* val); //return 0 on success
	int (*clock)(int clocks); //return 0 on success

	uint16_t* flash;
	uint8_t* data;
	uint8_t* eeprom;
	uint8_t* xmem;

	uint32_t flash_size;
	uint32_t data_size;
	uint32_t eeprom_size;
	uint32_t sram_base;
	uint32_t xmem_size;
};

int module_init(struct avr_core_s* core);
int module_load(char* dllfile);
void module_do_io_read(uint32_t addr, uint8_t* val);
void module_do_io_write(uint32_t addr, uint8_t* val);
void module_clock(int clocks);

#endif

