#ifndef _EEPROM_H_
#define _EEPROM_H_

#include <stdint.h>
#include "avrcore.h"

void eeprom_write(struct avr_core_s* core, void* dummy, uint32_t addr, uint8_t val);
uint8_t eeprom_read(struct avr_core_s* core, void* dummy, uint32_t addr);
void eeprom_clock(struct avr_core_s* core, void* dummy, int cycles);
int eeprom_init(struct avr_core_s* core, char* eep_file_name);

#endif
