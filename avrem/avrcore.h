#ifndef _AVRCORE_H_
#define _AVRCORE_H_

#include <stdio.h>
#include <stdint.h>
#include "avrregs.h"
#include "usart.h"

#define SREG_C			0
#define SREG_Z			1
#define SREG_N			2
#define SREG_V			3
#define SREG_S			4
#define SREG_H			5
#define SREG_T			6
#define SREG_I			7

#define REG(r) core->regaddr[r]

#define REG_SET_BIT(core, reg, bit) core->data[REG(reg)] |= (1 << bit)
#define REG_CLR_BIT(core, reg, bit) core->data[REG(reg)] &=~ (1 << bit)

#define GET_SREG_BIT(core, bit) ((core->data[REG(R_SREG)] & (1 << bit)) ? 1 : 0)

#define SIGNEXT7(val) (uint32_t)((val) | (((val) & 0x00000040) ? 0xFFFFFF80 : 0x00000000))
#define SIGNEXT12(val) (uint32_t)((val) | (((val) & 0x00000800) ? 0xFFFFF000 : 0x00000000))
#define SIGNEXTPC ((core->pc & (core->flash_size >> 1)) ? (0xFFFFFFFF & ~core->addr_mask) | core->pc : core->pc)

struct avr_core_s {
	uint16_t* flash;
	uint8_t* data;
	uint8_t* eeprom;
	uint8_t* xmem;

	uint32_t flash_size;
	uint32_t data_size;
	uint32_t eeprom_size;
	uint32_t sram_base;
	uint32_t xmem_size;

	uint32_t pc;
	uint32_t addr_size;
	uint32_t addr_mask;

	void (*io_write[0x200])(struct avr_core_s* core, void* peripheral, uint32_t addr, uint8_t val);
	uint8_t (*io_read[0x200])(struct avr_core_s* core, void* peripheral, uint32_t addr);
	void* peripheral[0x200];

	int skip_next;
	int skip_after_interrupt;
	int sleeping;
	int external;

	//EEPROM control registers
	uint8_t eearh;
	uint8_t eearl;
	uint8_t eedr;
	uint8_t eecr;

	FILE* eep_file;

	uint32_t regaddr[REG_COUNT];
	uint32_t vectaddr[VECT_COUNT];
};

extern int trace;

void avr_flash_write_byte(struct avr_core_s* core, uint32_t addr, uint8_t val);
void avr_flash_write(struct avr_core_s* core, uint32_t addr, uint16_t val);
void avr_reset(struct avr_core_s* core);
int avr_execute(struct avr_core_s* core, int cycles);
int avr_interrupt(struct avr_core_s* core, uint32_t vector); //returns a 1 if the interrupt was accepted, so the calling function knows to clear the corresponding flag

#endif
