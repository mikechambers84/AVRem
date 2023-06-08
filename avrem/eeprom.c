#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "loglevel.h"
#include "avrcore.h"

void eeprom_write(struct avr_core_s* core, void* dummy, uint32_t addr, uint8_t val) {
	uint32_t ea;

	ea = ((uint32_t)core->eearh << 8) | (uint32_t)core->eearl;
	ea %= core->eeprom_size;

	if (addr == REG(R_EEARH)) {
		core->eearh = val;
	}
	else if (addr == REG(R_EEARL)) {
		core->eearl = val;
	}
	else if (addr == REG(R_EECR)) {
		val &= 0x3F;
		core->eecr = val;
		if ((val & 0x06) == 0x06) { //EEPE and EEMPE set
			core->eeprom[ea] = core->eedr; //write data from EEDR
			core->eecr &= 0xF9; //clear EEPE and EEMPE
			if (core->eep_file != NULL) {
				fseek(core->eep_file, ea, SEEK_SET);
				fwrite(&core->eedr, 1, 1, core->eep_file);
				fflush(core->eep_file);
			}
			//printf("Write EEPROM %04X: %02X\n", ea, core->eedr);
		}
		if (val & 1) { //EERE
			core->eedr = core->eeprom[ea]; //put data in EEDR
			core->eecr &= 0xFE; //clear EERE
		}
	}
	else if (addr == REG(R_EEDR)) {
		core->eedr = val;
	}
}

uint8_t eeprom_read(struct avr_core_s* core, void* dummy, uint32_t addr) {
	uint8_t ret = 0;

	if (addr == REG(R_EEARH)) {
		ret = core->eearh;
	}
	else if (addr == REG(R_EEARL)) {
		ret = core->eearl;
	}
	else if (addr == REG(R_EECR)) {
		ret = core->eecr;
	}
	else if (addr == REG(R_EEDR)) {
		ret = core->eedr;
	}

	return ret;
}

void eeprom_clock(struct avr_core_s* core, void* dummy, int cycles) {
	//TODO: clock stuff and interrupt...
}

int eeprom_init(struct avr_core_s* core, char* eep_file_name) {
	size_t fs;
	core->eep_file = fopen(eep_file_name, "r+b");
	if (core->eep_file == NULL) {
		core->eep_file = fopen(eep_file_name, "w+b");
	}
	if (core->eep_file == NULL) {
		return -1;
	}

	fseek(core->eep_file, 0, SEEK_END);
	fs = ftell(core->eep_file);
	fseek(core->eep_file, 0, SEEK_SET);
	if (fs == 0) {
		fwrite(core->eeprom, 1, core->eeprom_size, core->eep_file);
	}
	else {
		fread(core->eeprom, 1, core->eeprom_size, core->eep_file);
	}
	fseek(core->eep_file, 0, SEEK_SET);

	return 0;
}
