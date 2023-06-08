#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "config.h"
#include "avrcore.h"
#include "adc.h"

void adc_write(struct avr_core_s* core, struct adc_s* adc, uint32_t addr, uint8_t val) {
	if (addr == adc->addr_adcsra) {
		int ps;
		uint8_t adsc_state_old, adsc_state_new;
		adsc_state_old = adc->adcsra & (1 << 6);
		adsc_state_new = val & (1 << 6);
		adc->adcsra = (val & ~(1 << 6)) | (adsc_state_new | adsc_state_old);
		if (val & (1 << 4)) adc->adcsra &= ~(1 << 4); //writing 1 to ADIF clears the flag
		if (adsc_state_new && !adsc_state_old && (val & (1 << 7))) { //ADSC -- start conversion (but only if ADEN is set)
			ps = val & 7;
			if (ps == 0) ps = 1;
			ps = (1 << ps);
			adc->sample_cycles = 13 * ps;
			//printf("started sample with %d clocks\n", adc->sample_cycles);
		}
		else if ((val & (1 << 7)) == 0) { //ADEN turned off
			adc->sample_cycles = 0;
		}
	}
	else if (addr == adc->addr_admux) {
		adc->admux = val;
	}
}

uint8_t adc_read(struct avr_core_s* core, struct adc_s* adc, uint32_t addr) {
	uint8_t ret = 0;
	uint16_t readval;

	readval = adc->adcsample;
	if (adc->admux & 0x20) {
		readval <<= 6; //ADLAR set means shift left 6 bits
	}

	if (addr == adc->addr_adch) {
		ret = (uint8_t)(readval >> 8);
	}
	else if (addr == adc->adcl) {
		ret = (uint8_t)readval;
	}
	else if (addr == adc->addr_adcsra) {
		ret = adc->adcsra;
	}
	else if (addr == adc->addr_admux) {
		ret = adc->admux;
	}

	return ret;
}

void adc_setinput(struct adc_s* adc, uint8_t channel, uint16_t val) {
	if (channel > 15) return;
	adc->adcinput[channel] = val;
}

void adc_clock(struct avr_core_s* core, struct adc_s* adc, int cycles) {
	if (adc == NULL) return;
	if ((adc->adcsra & (1 << 7)) == 0) return; //ADC not enabled
	if ((adc->adcsra & (1 << 6)) == 0) return; //not doing a conversion right now

adc_clock_again:
	if (adc->sample_cycles == 0) {
		adc->adcsra &= ~(1 << 6); //clear ADSC
		adc->adcsra |= (1 << 4); //set ADIF
		adc->adcsample = adc->adcinput[adc->admux & 0x0F];
		goto adc_clock_done;
	}

	adc->sample_cycles--;

	if (--cycles > 0) goto adc_clock_again;

adc_clock_done:
	//interrupts
	if ((adc->adcsra & (1 << 3)) && (adc->adcsra & (1 << 4))) { //ADIE and ADIF both set
		if (avr_interrupt(core, adc->int_vector_adc)) {
			adc->adcsra &= ~(1 << 4); //clear ADIF automatically only if interrupt executed
		}
	}

}

void adc_init(struct adc_s* adc) {
	adc->sample_cycles = 0;
	adc->adch = 0;
	adc->adcl = 0;
	adc->adcsra = 0;
	adc->admux = 0;
	//adc_setinput(adc, 7, 1023);
}
