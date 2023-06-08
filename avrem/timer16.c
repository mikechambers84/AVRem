#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "timer16.h"
#include "avrcore.h"

void timer16_write(struct avr_core_s* core, struct timer16_s* timer16, uint32_t addr, uint8_t val) {
	if (addr == timer16->addr_tccra) {
		timer16->tccra = val;
	}
	else if (addr == timer16->addr_tccrb) {
		timer16->tccrb = val & 0xDF; //mask off unwritable bits
	}
	else if (addr == timer16->addr_tccrc) {
		timer16->tccrc = val & 0xE0; //mask off unwritable bits
	}
	else if (addr == timer16->addr_tcntl) {
		timer16->tcntl = val;
		timer16->counter = (uint16_t)val | ((uint16_t)timer16->tcnth << 8);
	}
	else if (addr == timer16->addr_tcnth) {
		timer16->tcnth = val;
	}
	else if (addr == timer16->addr_icrl) {
		timer16->icrl = val;
	}
	else if (addr == timer16->addr_icrh) {
		timer16->icrh = val;
	}
	else if (addr == timer16->addr_ocral) {
		timer16->ocral = val;
	}
	else if (addr == timer16->addr_ocrah) {
		timer16->ocrah = val;
	}
	else if (addr == timer16->addr_ocrbl) {
		timer16->ocrbl = val;
	}
	else if (addr == timer16->addr_ocrbh) {
		timer16->ocrbh = val;
	}
	else if (addr == timer16->addr_ocrcl) {
		timer16->ocrcl = val;
	}
	else if (addr == timer16->addr_ocrch) {
		timer16->ocrch = val;
	}
	else if (addr == timer16->addr_timsk) {
		core->data[timer16->addr_timsk] = val;
	}
	else if (addr == timer16->addr_tifr) {
		core->data[timer16->addr_tifr] &= ~(val & core->data[timer16->addr_tifr]); //writing a one to any interrupt flag bit clears it
		//timer16->tifr &= 0x2F;
	}

	timer16->ocra = ((uint16_t)timer16->ocrah << 8) | (uint16_t)timer16->ocral;
	timer16->ocrb = ((uint16_t)timer16->ocrbh << 8) | (uint16_t)timer16->ocrbl;
	timer16->ocrc = ((uint16_t)timer16->ocrch << 8) | (uint16_t)timer16->ocrcl;
	timer16->mode = (timer16->tccra & 3) | ((timer16->tccrb & 0x18) >> 1);
	timer16->cs = timer16->tccrb & 7;
	switch (timer16->cs) {
	case 1: timer16->ps_setting = 1; break;
	case 2: timer16->ps_setting = 8; break;
	case 3: timer16->ps_setting = 64; break;
	case 4: timer16->ps_setting = 256; break;
	case 5: timer16->ps_setting = 1024; break;
	}

	/*printf("Timer16 OCRA: %04X\n", timer16->ocra);
	printf("Timer16 OCRB: %04X\n", timer16->ocrb);
	printf("Timer16 OCRC: %04X\n", timer16->ocrc);
	printf("Timer16 mode: %u\n", timer16->mode);
	printf("Timer16 clock source: %u\n\n", timer16->cs);*/
}

uint8_t timer16_read(struct avr_core_s* core, struct timer16_s* timer16, uint32_t addr) {
	uint8_t ret = 0x00;

	if (addr == timer16->addr_tccra) {
		ret = timer16->tccra;
	}
	else if (addr == timer16->addr_tccrb) {
		ret = timer16->tccrb;
	}
	else if (addr == timer16->addr_tccrc) {
		ret = timer16->tccrc;
	}
	else if (addr == timer16->addr_tcntl) {
		ret = timer16->tcntl;
	}
	else if (addr == timer16->addr_tcnth) {
		ret = timer16->tcnth;
	}
	else if (addr == timer16->addr_icrl) {
		ret = timer16->icrl;
	}
	else if (addr == timer16->addr_icrh) {
		ret = timer16->icrh;
	}
	else if (addr == timer16->addr_ocral) {
		ret = timer16->ocral;
	}
	else if (addr == timer16->addr_ocrah) {
		ret = timer16->ocrah;
	}
	else if (addr == timer16->addr_ocrbl) {
		ret = timer16->ocrbl;
	}
	else if (addr == timer16->addr_ocrbh) {
		ret = timer16->ocrbh;
	}
	else if (addr == timer16->addr_ocrcl) {
		ret = timer16->ocrcl;
	}
	else if (addr == timer16->addr_ocrch) {
		ret = timer16->ocrch;
	}
	else if (addr == timer16->addr_timsk) {
		ret = core->data[timer16->addr_tifr];
	}
	else if (addr == timer16->addr_tifr) {
		ret = core->data[timer16->addr_tifr];
	}

	return ret;
}

void timer16_clock(struct avr_core_s* core, struct timer16_s* timer16, int clocks) {
	if (timer16->cs == 0) return; //no clock source, timer disabled
	if (timer16->cs > 5) return; //TODO: external clock source, eventually

timer16_clock_again:
	if (timer16->ps_counter >= timer16->ps_setting) { //prescaler
		timer16->ps_counter = 0;
		switch (timer16->mode) {
		case 0: //Normal
			if (++timer16->counter == 0) {
				core->data[timer16->addr_tifr] |= (1 << timer16->bit_mask_ovf);
			}
			break;
		case 1: //PWM 8-bit
			if (++timer16->counter == 0x100) {
				timer16->counter = 0;
				core->data[timer16->addr_tifr] |= (1 << timer16->bit_mask_ovf);
			}
			break;
		case 4: //CTC OCR
			if (++timer16->counter == timer16->ocra) {
				timer16->counter = 0;
				core->data[timer16->addr_tifr] |= (1 << timer16->bit_mask_ocra);
			}
			break;
		default:
			printf("ERROR: 16-bit timer mode %u is unsupported in AVRem.\n", timer16->mode);
			exit(0);
		}
	}

	timer16->ps_counter++;

	if (--clocks > 0) goto timer16_clock_again;

	//interrupts
	if (core->data[timer16->addr_timsk] & core->data[timer16->addr_tifr] & (1 << timer16->bit_mask_ovf)) { //OVF
		if (avr_interrupt(core, timer16->int_vector_OVF)) {
			core->data[timer16->addr_tifr] &= ~(1 << timer16->bit_mask_ovf);
		}
	}
	else if (core->data[timer16->addr_timsk] & core->data[timer16->addr_tifr] & (1 << timer16->bit_mask_ocra)) { //OCRA
		if (avr_interrupt(core, timer16->int_vector_OCA)) {
			core->data[timer16->addr_tifr] &= ~(1 << timer16->bit_mask_ocra);
		}
	}
}

void timer16_init(struct timer16_s* timer16) {
	timer16->tccra = 0;
	timer16->tccrb = 0;
	timer16->tccrc = 0;
	timer16->tcnth = 0;
	timer16->tcntl = 0;
	timer16->ocrah = 0;
	timer16->ocral = 0;
	timer16->ocrbh = 0;
	timer16->ocrbl = 0;
	timer16->ocrch = 0;
	timer16->ocrcl = 0;
	timer16->icrh = 0;
	timer16->icrl = 0;
	timer16->tifr = 0;
	timer16->timsk = 0;
	timer16->counter = 0;
	timer16->top = 0xFFFF;
	timer16->ps_counter = 0;
	timer16->ps_setting = 1;
}
