#ifndef _TIMER16_H_
#define _TIMER16_H_

#include "avrcore.h"

struct timer16_s {
	uint8_t ocrch;
	uint8_t ocrcl;
	uint8_t ocrbh;
	uint8_t ocrbl;
	uint8_t ocrah;
	uint8_t ocral;

	uint8_t icrh;
	uint8_t icrl;

	uint8_t tcnth;
	uint8_t tcntl;

	uint8_t tccra;
	uint8_t tccrb;
	uint8_t tccrc;

	uint8_t timsk;
	uint8_t tifr;

	uint8_t mode;
	uint8_t cs;

	uint16_t ocra;
	uint16_t ocrb;
	uint16_t ocrc;

	uint16_t counter;
	uint16_t top;
	uint16_t ps_counter;
	uint16_t ps_setting;

	uint32_t int_vector_OCA;
	uint32_t int_vector_OCB;
	uint32_t int_vector_OCC;
	uint32_t int_vector_OVF;

	uint32_t addr_tifr;
	uint32_t addr_timsk;
	uint32_t addr_tccra;
	uint32_t addr_tccrb;
	uint32_t addr_tccrc;
	uint32_t addr_tcnth;
	uint32_t addr_tcntl;
	uint32_t addr_icrh;
	uint32_t addr_icrl;
	uint32_t addr_ocrch;
	uint32_t addr_ocrcl;
	uint32_t addr_ocrbh;
	uint32_t addr_ocrbl;
	uint32_t addr_ocrah;
	uint32_t addr_ocral;

	uint8_t bit_mask_ocra;
	uint8_t bit_mask_ovf;
};

void timer16_write(struct avr_core_s* core, struct timer16_s* timer16, uint32_t addr, uint8_t val);
uint8_t timer16_read(struct avr_core_s* core, struct timer16_s* timer16, uint32_t addr);
void timer16_clock(struct avr_core_s* core, struct timer16_s* timer16, int clocks);
void timer16_init(struct timer16_s* timer16);

#endif
