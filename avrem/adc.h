#ifndef _ADC_H_
#define _ADC_H_

#include <stdint.h>

struct adc_s {
	uint16_t adcinput[16];
	uint16_t adcsample;
	uint8_t admux;
	uint8_t adcsra;
	uint8_t adcl;
	uint8_t adch;

	int sample_cycles;

	uint32_t addr_admux;
	uint32_t addr_adcsra;
	uint32_t addr_adcl;
	uint32_t addr_adch;

	uint32_t int_vector_adc;
};

void adc_write(struct avr_core_s* core, struct adc_s* adc, uint32_t addr, uint8_t val);
uint8_t adc_read(struct avr_core_s* core, struct adc_s* adc, uint32_t addr);
void adc_setinput(struct adc_s* adc, uint8_t channel, uint16_t val);
void adc_clock(struct avr_core_s* core, struct adc_s* adc, int cycles);
void adc_init(struct adc_s* adc);

#endif
