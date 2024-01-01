#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "loglevel.h"
#include "avrcore.h"
#include "module.h"

struct module_s module[MODULES_MAX_COUNT];
int module_count = 0;

void module_do_io_read(uint32_t addr, uint8_t* val) {
	int i;
	for (i = 0; i < module_count; i++) {
		if (module[i].io_read != NULL) {
			(*module[i].io_read)(addr, val);
		}
	}
}

void module_do_io_write(uint32_t addr, uint8_t* val) {
	int i;
	for (i = 0; i < module_count; i++) {
		if (module[i].io_write != NULL) {
			(*module[i].io_write)(addr, val);
		}
	}
}

void module_clock(int clocks) {
	int i;
	for (i = 0; i < module_count; i++) {
		if (module[i].clock != NULL) {
			(*module[i].clock)(clocks);
		}
	}
}

int module_init(struct avr_core_s* core) {
	int i;

	for (i = 0; i < module_count; i++) {
		module[i].flash = core->flash;
		module[i].data = core->data;
		module[i].eeprom = core->eeprom;
		module[i].xmem = core->xmem;
		module[i].flash_size = core->flash_size;
		module[i].data_size = core->data_size;
		module[i].eeprom_size = core->eeprom_size;
		module[i].xmem_size = core->xmem_size;

		if (module[i].init != NULL) {
			if ((*module[i].init)(&module[i])) {
				FreeLibrary(module[i].instance);
				printl(LOG_ERROR, "Module init function returned failure status (%s)\n", module[i].name);
				module[i].io_read = module[i].io_write = NULL;
				module[i].clock = NULL;
			}
			else {
				printl(LOG_INFO, "Module initialized: %c%s%c\n", 34, module[i].name, 34);
			}
		}
		else {
			FreeLibrary(module[i].instance);
			printl(LOG_ERROR, "Module library contains no init function, so not loaded (%s)\n", module[i].name);
			module[i].io_read = module[i].io_write = NULL;
			module[i].clock = NULL;
		}
	}

	return 0;
}

int module_load(char* dllfile) {
	if (module_count == MODULES_MAX_COUNT) {
		return -1;
	}

	module[module_count].name = dllfile; //something in case init fails later
	module[module_count].instance = LoadLibraryA(dllfile);
	if (module[module_count].instance == NULL) {
		return -1;
	}

	module[module_count].init = (void *)GetProcAddress(module[module_count].instance, "init");
	module[module_count].io_read = (void *)GetProcAddress(module[module_count].instance, "io_read");
	module[module_count].io_write = (void *)GetProcAddress(module[module_count].instance, "io_write");
	module[module_count].clock = (void *)GetProcAddress(module[module_count].instance, "clock");

	module_count++;
	return 0;
}

#endif
