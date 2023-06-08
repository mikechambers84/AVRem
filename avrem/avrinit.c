#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "args.h"
#include "loglevel.h"
#include "avrcore.h"
#include "tcpconsole.h"
#include "eeprom.h"
#include "usart.h"
#include "timer16.h"
#include "adc.h"
#include "periph.h"

struct timer16_s timer16_1;
struct timer16_s timer16_3;
struct timer16_s timer16_4;
struct timer16_s timer16_5;
struct usart_s usart[USART_MAX_COUNT];
struct adc_s adc;

struct avr_model_s {
	char* name;
	uint32_t flash_size;
	uint32_t data_size;
	uint32_t eeprom_size;
	uint64_t peripherals; //add peripheral options later
	uint32_t sram_base;
	void (*device_init)(struct avr_core_s* core);
};

struct avr_model_s models[] = {
	{
		.name = "atmega8",
		.flash_size = 8192,
		.data_size = 1024,
		.eeprom_size = 512,
		.peripherals = 0,
		.sram_base = 0x60,
		.device_init = init_regs_8
	},
	{
		.name = "atmega16",
		.flash_size = 16384,
		.data_size = 1024,
		.eeprom_size = 512,
		.peripherals = 0,
		.sram_base = 0x60,
		.device_init = init_regs_16
	},
	{
		.name = "atmega32",
		.flash_size = 32768,
		.data_size = 2048,
		.eeprom_size = 1024,
		.peripherals = 0,
		.sram_base = 0x60,
		.device_init = init_regs_32
	},
	{
		.name = "atmega64",
		.flash_size = 65536,
		.data_size = 4096,
		.eeprom_size = 2048,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_64
	},
	{
		.name = "atmega48",
		.flash_size = 4096,
		.data_size = 512,
		.eeprom_size = 256,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_48_88_168_328
	},
	{
		.name = "atmega88",
		.flash_size = 8192,
		.data_size = 1024,
		.eeprom_size = 512,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_48_88_168_328
	},
	{
		.name = "atmega168",
		.flash_size = 16384,
		.data_size = 1024,
		.eeprom_size = 512,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_48_88_168_328
	},
	{
		.name = "atmega328",
		.flash_size = 32768,
		.data_size = 2048,
		.eeprom_size = 1024,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_48_88_168_328
	},
	{
		.name = "atmega164",
		.flash_size = 16384,
		.data_size = 1024,
		.eeprom_size = 512,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_164_324_644_1284
	},
	{
		.name = "atmega324",
		.flash_size = 32768,
		.data_size = 2048,
		.eeprom_size = 1024,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_164_324_644_1284
	},
	{
		.name = "atmega644",
		.flash_size = 65536,
		.data_size = 4096,
		.eeprom_size = 2048,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_164_324_644_1284
	},
	{
		.name = "atmega1284",
		.flash_size = 131072,
		.data_size = 16384,
		.eeprom_size = 4096,
		.peripherals = 0,
		.sram_base = 0x100,
		.device_init = init_regs_164_324_644_1284
	},
	{
		.name = "atmega640",
		.flash_size = 65536,
		.data_size = 8192,
		.eeprom_size = 4096,
		.peripherals = 0,
		.sram_base = 0x200,
		.device_init = init_regs_640_1280_1281_2560_2561
	},
	{
		.name = "atmega1280",
		.flash_size = 131072,
		.data_size = 8192,
		.eeprom_size = 4096,
		.peripherals = 0,
		.sram_base = 0x200,
		.device_init = init_regs_640_1280_1281_2560_2561
	},
	{
		.name = "atmega1281",
		.flash_size = 131072,
		.data_size = 8192,
		.eeprom_size = 4096,
		.peripherals = 0,
		.sram_base = 0x200,
		.device_init = init_regs_640_1280_1281_2560_2561
	},
	{
		.name = "atmega2560",
		.flash_size = 262144,
		.data_size = 8192,
		.eeprom_size = 4096,
		.peripherals = 0,
		.sram_base = 0x200,
		.device_init = init_regs_640_1280_1281_2560_2561
	},
	{
		.name = "atmega2561",
		.flash_size = 262144,
		.data_size = 8192,
		.eeprom_size = 4096,
		.peripherals = 0,
		.sram_base = 0x200,
		.device_init = init_regs_640_1280_1281_2560_2561
	},
	{
		.name = NULL
	}
};

void init_regs_16(struct avr_core_s* core) {
	core->regaddr[R_SREG] = 0xF;
	core->regaddr[R_SPH] = 0xE;
	core->regaddr[R_SPL] = 0xD;
	core->regaddr[R_OCR0] = 0xC;
	core->regaddr[R_GICR] = 0xB;
	core->regaddr[R_GIFR] = 0xA;
	core->regaddr[R_TIMSK] = 0x9;
	core->regaddr[R_TIFR] = 0x8;
	core->regaddr[R_SPMCR] = 0x7;
	core->regaddr[R_TWCR] = 0x6;
	core->regaddr[R_MCUCR] = 0x5;
	core->regaddr[R_MCUCSR] = 0x4;
	core->regaddr[R_TCCR0] = 0x3;
	core->regaddr[R_TCNT0] = 0x2;
	core->regaddr[R_OSCCAL] = 0x1;
	core->regaddr[R_OCDR] = 0x1;
	core->regaddr[R_SFIOR] = 0x0;
	core->regaddr[R_TCCR1A] = 0xF;
	core->regaddr[R_TCCR1B] = 0xE;
	core->regaddr[R_TCNT1H] = 0xD;
	core->regaddr[R_TCNT1L] = 0xC;
	core->regaddr[R_OCR1AH] = 0xB;
	core->regaddr[R_OCR1AL] = 0xA;
	core->regaddr[R_OCR1BH] = 0x9;
	core->regaddr[R_OCR1BL] = 0x8;
	core->regaddr[R_ICR1H] = 0x7;
	core->regaddr[R_ICR1L] = 0x6;
	core->regaddr[R_TCCR2] = 0x5;
	core->regaddr[R_TCNT2] = 0x4;
	core->regaddr[R_OCR2] = 0x3;
	core->regaddr[R_ASSR] = 0x2;
	core->regaddr[R_WDTCR] = 0x1;
	core->regaddr[R_UBRRH] = 0x0;
	core->regaddr[R_UCSRC] = 0x0;
	core->regaddr[R_EEARH] = 0xF;
	core->regaddr[R_EEARL] = 0xE;
	core->regaddr[R_EEDR] = 0xD;
	core->regaddr[R_EECR] = 0xC;
	core->regaddr[R_PORTA] = 0xB;
	core->regaddr[R_DDRA] = 0xA;
	core->regaddr[R_PINA] = 0x9;
	core->regaddr[R_PORTB] = 0x8;
	core->regaddr[R_DDRB] = 0x7;
	core->regaddr[R_PINB] = 0x6;
	core->regaddr[R_PORTC] = 0x5;
	core->regaddr[R_DDRC] = 0x4;
	core->regaddr[R_PINC] = 0x3;
	core->regaddr[R_PORTD] = 0x2;
	core->regaddr[R_DDRD] = 0x1;
	core->regaddr[R_PIND] = 0x0;
	core->regaddr[R_SPDR] = 0xF;
	core->regaddr[R_SPSR] = 0xE;
	core->regaddr[R_SPCR] = 0xD;
	core->regaddr[R_UDR] = 0xC;
	core->regaddr[R_UCSRA] = 0xB;
	core->regaddr[R_UCSRB] = 0xA;
	core->regaddr[R_UBRRL] = 0x9;
	core->regaddr[R_ACSR] = 0x8;
	core->regaddr[R_ADMUX] = 0x7;
	core->regaddr[R_ADCSRA] = 0x6;
	core->regaddr[R_ADCH] = 0x5;
	core->regaddr[R_ADCL] = 0x4;
	core->regaddr[R_TWDR] = 0x3;
	core->regaddr[R_TWAR] = 0x2;
	core->regaddr[R_TWSR] = 0x1;
	core->regaddr[R_TWBR] = 0x0;

	core->vectaddr[VECT_RESET] = 0x0;
	core->vectaddr[VECT_INT0] = 0x2;
	core->vectaddr[VECT_INT1] = 0x4;
	core->vectaddr[VECT_TIMER2_COMP] = 0x6;
	core->vectaddr[VECT_TIMER2_OVF] = 0x8;
	core->vectaddr[VECT_TIMER1_CAPT] = 0xA;
	core->vectaddr[VECT_TIMER1_COMPA] = 0xC;
	core->vectaddr[VECT_TIMER1_COMPB] = 0xE;
	core->vectaddr[VECT_TIMER1_OVF] = 0x10;
	core->vectaddr[VECT_TIMER0_OVF] = 0x12;
	core->vectaddr[VECT_SPI_STC] = 0x14;
	core->vectaddr[VECT_USART_RXC] = 0x16;
	core->vectaddr[VECT_USART_UDRE] = 0x18;
	core->vectaddr[VECT_USART_TXC] = 0x1A;
	core->vectaddr[VECT_ADC] = 0x1C;
	core->vectaddr[VECT_EE_RDY] = 0x1E;
	core->vectaddr[VECT_ANA_COMP] = 0x20;
	core->vectaddr[VECT_TWI] = 0x22;
	core->vectaddr[VECT_INT2] = 0x24;
	core->vectaddr[VECT_TIMER0_COMP] = 0x26;
	core->vectaddr[VECT_SPM_RDY] = 0x28;
}

void init_regs_164_324_644_1284(struct avr_core_s* core) {
	core->regaddr[R_UDR1] = 0xCE;
	core->regaddr[R_UBRR1H] = 0xCD;
	core->regaddr[R_UBRR1L] = 0xCC;
	core->regaddr[R_UCSR1C] = 0xCA;
	core->regaddr[R_UCSR1B] = 0xC9;
	core->regaddr[R_UCSR1A] = 0xC8;
	core->regaddr[R_UDR0] = 0xC6;
	core->regaddr[R_UBRR0H] = 0xC5;
	core->regaddr[R_UBRR0L] = 0xC4;
	core->regaddr[R_UCSR0C] = 0xC2;
	core->regaddr[R_UCSR0B] = 0xC1;
	core->regaddr[R_UCSR0A] = 0xC0;
	core->regaddr[R_TWAMR] = 0xBD;
	core->regaddr[R_TWCR] = 0xBC;
	core->regaddr[R_TWDR] = 0xBB;
	core->regaddr[R_TWAR] = 0xBA;
	core->regaddr[R_TWSR] = 0xB9;
	core->regaddr[R_TWBR] = 0xB8;
	core->regaddr[R_ASSR] = 0xB6;
	core->regaddr[R_OCR2B] = 0xB4;
	core->regaddr[R_OCR2A] = 0xB3;
	core->regaddr[R_TCNT2] = 0xB2;
	core->regaddr[R_TCCR2B] = 0xB1;
	core->regaddr[R_TCCR2A] = 0xB0;
	core->regaddr[R_OCR3BH] = 0x9B;
	core->regaddr[R_OCR3BL] = 0x9A;
	core->regaddr[R_OCR3AH] = 0x99;
	core->regaddr[R_OCR3AL] = 0x98;
	core->regaddr[R_ICR3H] = 0x97;
	core->regaddr[R_ICR3L] = 0x96;
	core->regaddr[R_TCNT3H] = 0x95;
	core->regaddr[R_TCNT3L] = 0x94;
	core->regaddr[R_TCCR3C] = 0x92;
	core->regaddr[R_TCCR3B] = 0x91;
	core->regaddr[R_TCCR3A] = 0x90;
	core->regaddr[R_OCR1BH] = 0x8B;
	core->regaddr[R_OCR1BL] = 0x8A;
	core->regaddr[R_OCR1AH] = 0x89;
	core->regaddr[R_OCR1AL] = 0x88;
	core->regaddr[R_ICR1H] = 0x87;
	core->regaddr[R_ICR1L] = 0x86;
	core->regaddr[R_TCNT1H] = 0x85;
	core->regaddr[R_TCNT1L] = 0x84;
	core->regaddr[R_TCCR1C] = 0x82;
	core->regaddr[R_TCCR1B] = 0x81;
	core->regaddr[R_TCCR1A] = 0x80;
	core->regaddr[R_DIDR1] = 0x7F;
	core->regaddr[R_DIDR0] = 0x7E;
	core->regaddr[R_ADMUX] = 0x7C;
	core->regaddr[R_ADCSRB] = 0x7B;
	core->regaddr[R_ADCSRA] = 0x7A;
	core->regaddr[R_ADCH] = 0x79;
	core->regaddr[R_ADCL] = 0x78;
	core->regaddr[R_PCMSK3] = 0x73;
	core->regaddr[R_TIMSK3] = 0x71;
	core->regaddr[R_TIMSK2] = 0x70;
	core->regaddr[R_TIMSK1] = 0x6F;
	core->regaddr[R_TIMSK0] = 0x6E;
	core->regaddr[R_PCMSK2] = 0x6D;
	core->regaddr[R_PCMSK1] = 0x6C;
	core->regaddr[R_PCMSK0] = 0x6B;
	core->regaddr[R_EICRA] = 0x69;
	core->regaddr[R_PCICR] = 0x68;
	core->regaddr[R_OSCCAL] = 0x66;
	core->regaddr[R_PRR1] = 0x65;
	core->regaddr[R_PRR0] = 0x64;
	core->regaddr[R_CLKPR] = 0x61;
	core->regaddr[R_WDTCSR] = 0x60;
	core->regaddr[R_SREG] = 0x5F;
	core->regaddr[R_SPH] = 0x5E;
	core->regaddr[R_SPL] = 0x5D;
	core->regaddr[R_SPMCSR] = 0x57;
	core->regaddr[R_MCUCR] = 0x55;
	core->regaddr[R_MCUSR] = 0x54;
	core->regaddr[R_SMCR] = 0x53;
	core->regaddr[R_OCDR] = 0x51;
	core->regaddr[R_ACSR] = 0x50;
	core->regaddr[R_SPDR] = 0x4E;
	core->regaddr[R_SPSR] = 0x4D;
	core->regaddr[R_SPCR] = 0x4C;
	core->regaddr[R_GPIOR2] = 0x4B;
	core->regaddr[R_GPIOR1] = 0x4A;
	core->regaddr[R_OCR0B] = 0x48;
	core->regaddr[R_OCR0A] = 0x47;
	core->regaddr[R_TCNT0] = 0x46;
	core->regaddr[R_TCCR0B] = 0x45;
	core->regaddr[R_TCCR0A] = 0x44;
	core->regaddr[R_GTCCR] = 0x43;
	core->regaddr[R_EEARH] = 0x42;
	core->regaddr[R_EEARL] = 0x41;
	core->regaddr[R_EEDR] = 0x40;
	core->regaddr[R_EECR] = 0x3F;
	core->regaddr[R_GPIOR0] = 0x3E;
	core->regaddr[R_EIMSK] = 0x3D;
	core->regaddr[R_EIFR] = 0x3C;
	core->regaddr[R_PCIFR] = 0x3B;
	core->regaddr[R_TIFR3] = 0x38;
	core->regaddr[R_TIFR2] = 0x37;
	core->regaddr[R_TIFR1] = 0x36;
	core->regaddr[R_TIFR0] = 0x35;
	core->regaddr[R_PORTD] = 0x2B;
	core->regaddr[R_DDRD] = 0x2A;
	core->regaddr[R_PIND] = 0x29;
	core->regaddr[R_PORTC] = 0x28;
	core->regaddr[R_DDRC] = 0x27;
	core->regaddr[R_PINC] = 0x26;
	core->regaddr[R_PORTB] = 0x25;
	core->regaddr[R_DDRB] = 0x24;
	core->regaddr[R_PINB] = 0x23;
	core->regaddr[R_PORTA] = 0x22;
	core->regaddr[R_DDRA] = 0x21;
	core->regaddr[R_PINA] = 0x20;

	core->vectaddr[VECT_RESET] = 0x0;
	core->vectaddr[VECT_INT0] = 0x2;
	core->vectaddr[VECT_INT1] = 0x4;
	core->vectaddr[VECT_INT2] = 0x6;
	core->vectaddr[VECT_PCINT0] = 0x8;
	core->vectaddr[VECT_PCINT1] = 0xA;
	core->vectaddr[VECT_PCINT2] = 0xC;
	core->vectaddr[VECT_PCINT3] = 0xE;
	core->vectaddr[VECT_WDT] = 0x10;
	core->vectaddr[VECT_TIMER2_COMPA] = 0x12;
	core->vectaddr[VECT_TIMER2_COMPB] = 0x14;
	core->vectaddr[VECT_TIMER2_OVF] = 0x16;
	core->vectaddr[VECT_TIMER1_CAPT] = 0x18;
	core->vectaddr[VECT_TIMER1_COMPA] = 0x1A;
	core->vectaddr[VECT_TIMER1_COMPB] = 0x1C;
	core->vectaddr[VECT_TIMER1_OVF] = 0x1E;
	core->vectaddr[VECT_TIMER0_COMPA] = 0x20;
	core->vectaddr[VECT_TIMER0_COMPB] = 0x22;
	core->vectaddr[VECT_TIMER0_OVF] = 0x24;
	core->vectaddr[VECT_SPI_STC] = 0x26;
	core->vectaddr[VECT_USART0_RXC] = 0x28;
	core->vectaddr[VECT_USART0_UDRE] = 0x2A;
	core->vectaddr[VECT_USART0_TXC] = 0x2C;
	core->vectaddr[VECT_ANALOG_COMP] = 0x2E;
	core->vectaddr[VECT_ADC] = 0x30;
	core->vectaddr[VECT_EE_READY] = 0x32;
	core->vectaddr[VECT_TWI] = 0x34;
	core->vectaddr[VECT_SPM_READY] = 0x36;
	core->vectaddr[VECT_USART1_RXC] = 0x38;
	core->vectaddr[VECT_USART1_UDRE] = 0x3A;
	core->vectaddr[VECT_USART1_TXC] = 0x3C;
	core->vectaddr[VECT_TIMER3_CAPT] = 0x3E;
	core->vectaddr[VECT_TIMER3_COMPA] = 0x40;
	core->vectaddr[VECT_TIMER3_COMPB] = 0x42;
	core->vectaddr[VECT_TIMER3_OVF] = 0x44;

	//EEPROM
	core->io_write[REG(R_EEARH)] = eeprom_write; core->io_read[REG(R_EEARH)] = eeprom_read;
	core->io_write[REG(R_EEARL)] = eeprom_write; core->io_read[REG(R_EEARL)] = eeprom_read;
	core->io_write[REG(R_EEDR)] = eeprom_write; core->io_read[REG(R_EEDR)] = eeprom_read;
	core->io_write[REG(R_EECR)] = eeprom_write; core->io_read[REG(R_EECR)] = eeprom_read;
	periph_add_clock(NULL, eeprom_clock);

	//TIMER1
	timer16_init(&timer16_1);
	timer16_1.int_vector_OCA = core->vectaddr[VECT_TIMER1_COMPA];
	timer16_1.int_vector_OCB = core->vectaddr[VECT_TIMER1_COMPB];
	timer16_1.int_vector_OVF = core->vectaddr[VECT_TIMER1_OVF];
	timer16_1.addr_ocrbl = REG(R_OCR1BL);
	timer16_1.addr_ocrbh = REG(R_OCR1BH);
	timer16_1.addr_ocral = REG(R_OCR1AL);
	timer16_1.addr_ocrah = REG(R_OCR1AH);
	timer16_1.addr_ocrbl = REG(R_ICR1L);
	timer16_1.addr_ocrbh = REG(R_ICR1H);
	timer16_1.addr_tccra = REG(R_TCCR1A);
	timer16_1.addr_tccrb = REG(R_TCCR1B);
	timer16_1.addr_tcntl = REG(R_TCNT1L);
	timer16_1.addr_tcnth = REG(R_TCNT1H);
	timer16_1.addr_timsk = REG(R_TIMSK1);
	timer16_1.addr_tifr = REG(R_TIFR1);
	timer16_1.bit_mask_ocra = 1;
	timer16_1.bit_mask_ovf = 0;
	core->io_write[REG(R_OCR1BL)] = timer16_write; core->io_read[REG(R_OCR1BL)] = timer16_read; core->peripheral[REG(R_OCR1BL)] = &timer16_1;
	core->io_write[REG(R_OCR1BH)] = timer16_write; core->io_read[REG(R_OCR1BH)] = timer16_read; core->peripheral[REG(R_OCR1BH)] = &timer16_1;
	core->io_write[REG(R_OCR1AL)] = timer16_write; core->io_read[REG(R_OCR1AL)] = timer16_read; core->peripheral[REG(R_OCR1AL)] = &timer16_1;
	core->io_write[REG(R_OCR1AH)] = timer16_write; core->io_read[REG(R_OCR1AH)] = timer16_read; core->peripheral[REG(R_OCR1AH)] = &timer16_1;
	core->io_write[REG(R_ICR1L)] = timer16_write; core->io_read[REG(R_ICR1L)] = timer16_read; core->peripheral[REG(R_ICR1L)] = &timer16_1;
	core->io_write[REG(R_ICR1H)] = timer16_write; core->io_read[REG(R_ICR1H)] = timer16_read; core->peripheral[REG(R_ICR1H)] = &timer16_1;
	core->io_write[REG(R_TCCR1A)] = timer16_write; core->io_read[REG(R_TCCR1A)] = timer16_read; core->peripheral[REG(R_TCCR1A)] = &timer16_1;
	core->io_write[REG(R_TCCR1B)] = timer16_write; core->io_read[REG(R_TCCR1B)] = timer16_read; core->peripheral[REG(R_TCCR1B)] = &timer16_1;
	core->io_write[REG(R_TCNT1L)] = timer16_write; core->io_read[REG(R_TCNT1L)] = timer16_read; core->peripheral[REG(R_TCNT1L)] = &timer16_1;
	core->io_write[REG(R_TCNT1H)] = timer16_write; core->io_read[REG(R_TCNT1H)] = timer16_read; core->peripheral[REG(R_TCNT1H)] = &timer16_1;
	core->io_write[REG(R_TIMSK1)] = timer16_write; core->io_read[REG(R_TIMSK1)] = timer16_read; core->peripheral[REG(R_TIMSK1)] = &timer16_1;
	core->io_write[REG(R_TIFR1)] = timer16_write; core->io_read[REG(R_TIFR1)] = timer16_read; core->peripheral[REG(R_TIFR1)] = &timer16_1;
	periph_add_clock(&timer16_1, timer16_clock);

	//TIMER3
	timer16_init(&timer16_3);
	timer16_3.int_vector_OCA = core->vectaddr[VECT_TIMER3_COMPA];
	timer16_3.int_vector_OCB = core->vectaddr[VECT_TIMER3_COMPB];
	timer16_3.int_vector_OVF = core->vectaddr[VECT_TIMER3_OVF];
	timer16_3.addr_ocrbl = REG(R_OCR3BL);
	timer16_3.addr_ocrbh = REG(R_OCR3BH);
	timer16_3.addr_ocral = REG(R_OCR3AL);
	timer16_3.addr_ocrah = REG(R_OCR3AH);
	timer16_3.addr_ocrbl = REG(R_ICR3L);
	timer16_3.addr_ocrbh = REG(R_ICR3H);
	timer16_3.addr_tccra = REG(R_TCCR3A);
	timer16_3.addr_tccrb = REG(R_TCCR3B);
	timer16_3.addr_tcntl = REG(R_TCNT3L);
	timer16_3.addr_tcnth = REG(R_TCNT3H);
	timer16_3.addr_timsk = REG(R_TIMSK3);
	timer16_3.addr_tifr = REG(R_TIFR3);
	timer16_3.bit_mask_ocra = 1;
	timer16_3.bit_mask_ovf = 0;
	core->io_write[REG(R_OCR3BL)] = timer16_write; core->io_read[REG(R_OCR3BL)] = timer16_read; core->peripheral[REG(R_OCR3BL)] = &timer16_3;
	core->io_write[REG(R_OCR3BH)] = timer16_write; core->io_read[REG(R_OCR3BH)] = timer16_read; core->peripheral[REG(R_OCR3BH)] = &timer16_3;
	core->io_write[REG(R_OCR3AL)] = timer16_write; core->io_read[REG(R_OCR3AL)] = timer16_read; core->peripheral[REG(R_OCR3AL)] = &timer16_3;
	core->io_write[REG(R_OCR3AH)] = timer16_write; core->io_read[REG(R_OCR3AH)] = timer16_read; core->peripheral[REG(R_OCR3AH)] = &timer16_3;
	core->io_write[REG(R_ICR3L)] = timer16_write; core->io_read[REG(R_ICR3L)] = timer16_read; core->peripheral[REG(R_ICR3L)] = &timer16_3;
	core->io_write[REG(R_ICR3H)] = timer16_write; core->io_read[REG(R_ICR3H)] = timer16_read; core->peripheral[REG(R_ICR3H)] = &timer16_3;
	core->io_write[REG(R_TCCR3A)] = timer16_write; core->io_read[REG(R_TCCR3A)] = timer16_read; core->peripheral[REG(R_TCCR3A)] = &timer16_3;
	core->io_write[REG(R_TCCR3B)] = timer16_write; core->io_read[REG(R_TCCR3B)] = timer16_read; core->peripheral[REG(R_TCCR3B)] = &timer16_3;
	core->io_write[REG(R_TCNT3L)] = timer16_write; core->io_read[REG(R_TCNT3L)] = timer16_read; core->peripheral[REG(R_TCNT3L)] = &timer16_3;
	core->io_write[REG(R_TCNT3H)] = timer16_write; core->io_read[REG(R_TCNT3H)] = timer16_read; core->peripheral[REG(R_TCNT3H)] = &timer16_3;
	core->io_write[REG(R_TIMSK3)] = timer16_write; core->io_read[REG(R_TIMSK3)] = timer16_read; core->peripheral[REG(R_TIMSK3)] = &timer16_3;
	core->io_write[REG(R_TIFR3)] = timer16_write; core->io_read[REG(R_TIFR3)] = timer16_read; core->peripheral[REG(R_TIFR3)] = &timer16_3;
	periph_add_clock(&timer16_3, timer16_clock);

}

void init_regs_48_88_168_328(struct avr_core_s* core) {
	core->regaddr[R_UDR0] = 0xC6;
	core->regaddr[R_UBRR0H] = 0xC5;
	core->regaddr[R_UBRR0L] = 0xC4;
	core->regaddr[R_UCSR0C] = 0xC2;
	core->regaddr[R_UCSR0B] = 0xC1;
	core->regaddr[R_UCSR0A] = 0xC0;
	core->regaddr[R_TWAMR] = 0xBD;
	core->regaddr[R_TWCR] = 0xBC;
	core->regaddr[R_TWDR] = 0xBB;
	core->regaddr[R_TWAR] = 0xBA;
	core->regaddr[R_TWSR] = 0xB9;
	core->regaddr[R_TWBR] = 0xB8;
	core->regaddr[R_ASSR] = 0xB6;
	core->regaddr[R_OCR2B] = 0xB4;
	core->regaddr[R_OCR2A] = 0xB3;
	core->regaddr[R_TCNT2] = 0xB2;
	core->regaddr[R_TCCR2B] = 0xB1;
	core->regaddr[R_TCCR2A] = 0xB0;
	core->regaddr[R_OCR1BH] = 0x8B;
	core->regaddr[R_OCR1BL] = 0x8A;
	core->regaddr[R_OCR1AH] = 0x89;
	core->regaddr[R_OCR1AL] = 0x88;
	core->regaddr[R_ICR1H] = 0x87;
	core->regaddr[R_ICR1L] = 0x86;
	core->regaddr[R_TCNT1H] = 0x85;
	core->regaddr[R_TCNT1L] = 0x84;
	core->regaddr[R_TCCR1C] = 0x82;
	core->regaddr[R_TCCR1B] = 0x81;
	core->regaddr[R_TCCR1A] = 0x80;
	core->regaddr[R_DIDR1] = 0x7F;
	core->regaddr[R_DIDR0] = 0x7E;
	core->regaddr[R_ADMUX] = 0x7C;
	core->regaddr[R_ADCSRB] = 0x7B;
	core->regaddr[R_ADCSRA] = 0x7A;
	core->regaddr[R_ADCH] = 0x79;
	core->regaddr[R_ADCL] = 0x78;
	core->regaddr[R_TIMSK2] = 0x70;
	core->regaddr[R_TIMSK1] = 0x6F;
	core->regaddr[R_TIMSK0] = 0x6E;
	core->regaddr[R_PCMSK2] = 0x6D;
	core->regaddr[R_PCMSK1] = 0x6C;
	core->regaddr[R_PCMSK0] = 0x6B;
	core->regaddr[R_EICRA] = 0x69;
	core->regaddr[R_PCICR] = 0x68;
	core->regaddr[R_OSCCAL] = 0x66;
	core->regaddr[R_PRR] = 0x64;
	core->regaddr[R_CLKPR] = 0x61;
	core->regaddr[R_WDTCSR] = 0x60;
	core->regaddr[R_SREG] = 0x5F;
	core->regaddr[R_SPH] = 0x5E;
	core->regaddr[R_SPL] = 0x5D;
	core->regaddr[R_SPMCSR] = 0x57;
	core->regaddr[R_MCUCR] = 0x55;
	core->regaddr[R_MCUSR] = 0x54;
	core->regaddr[R_SMCR] = 0x53;
	core->regaddr[R_ACSR] = 0x50;
	core->regaddr[R_SPDR] = 0x4E;
	core->regaddr[R_SPSR] = 0x4D;
	core->regaddr[R_SPCR] = 0x4C;
	core->regaddr[R_GPIOR2] = 0x4B;
	core->regaddr[R_GPIOR1] = 0x4A;
	core->regaddr[R_OCR0B] = 0x48;
	core->regaddr[R_OCR0A] = 0x47;
	core->regaddr[R_TCNT0] = 0x46;
	core->regaddr[R_TCCR0B] = 0x45;
	core->regaddr[R_TCCR0A] = 0x44;
	core->regaddr[R_GTCCR] = 0x43;
	core->regaddr[R_EEARH] = 0x42;
	core->regaddr[R_EEARL] = 0x41;
	core->regaddr[R_EEDR] = 0x40;
	core->regaddr[R_EECR] = 0x3F;
	core->regaddr[R_GPIOR0] = 0x3E;
	core->regaddr[R_EIMSK] = 0x3D;
	core->regaddr[R_EIFR] = 0x3C;
	core->regaddr[R_PCIFR] = 0x3B;
	core->regaddr[R_TIFR2] = 0x37;
	core->regaddr[R_TIFR1] = 0x36;
	core->regaddr[R_TIFR0] = 0x35;
	core->regaddr[R_PORTD] = 0x2B;
	core->regaddr[R_DDRD] = 0x2A;
	core->regaddr[R_PIND] = 0x29;
	core->regaddr[R_PORTC] = 0x28;
	core->regaddr[R_DDRC] = 0x27;
	core->regaddr[R_PINC] = 0x26;
	core->regaddr[R_PORTB] = 0x25;
	core->regaddr[R_DDRB] = 0x24;
	core->regaddr[R_PINB] = 0x23;

	core->vectaddr[VECT_RESET] = 0x000;
	core->vectaddr[VECT_INT0] = 0x002;
	core->vectaddr[VECT_INT1] = 0x004;
	core->vectaddr[VECT_PCINT0] = 0x006;
	core->vectaddr[VECT_PCINT1] = 0x008;
	core->vectaddr[VECT_PCINT2] = 0x00A;
	core->vectaddr[VECT_WDT] = 0x00C;
	core->vectaddr[VECT_TIMER2_COMPA] = 0x00E;
	core->vectaddr[VECT_TIMER2_COMPB] = 0x010;
	core->vectaddr[VECT_TIMER2_OVF] = 0x012;
	core->vectaddr[VECT_TIMER1_CAPT] = 0x014;
	core->vectaddr[VECT_TIMER1_COMPA] = 0x016;
	core->vectaddr[VECT_TIMER1_COMPB] = 0x018;
	core->vectaddr[VECT_TIMER1_OVF] = 0x01A;
	core->vectaddr[VECT_TIMER0_COMPA] = 0x01C;
	core->vectaddr[VECT_TIMER0_COMPB] = 0x01E;
	core->vectaddr[VECT_TIMER0_OVF] = 0x020;
	core->vectaddr[VECT_SPI_STC] = 0x022;
	core->vectaddr[VECT_USART0_RXC] = 0x024;
	core->vectaddr[VECT_USART0_UDRE] = 0x026;
	core->vectaddr[VECT_USART0_TXC] = 0x028;
	core->vectaddr[VECT_ADC] = 0x02A;
	core->vectaddr[VECT_EE_READY] = 0x02C;
	core->vectaddr[VECT_ANALOG_COMP] = 0x02E;
	core->vectaddr[VECT_TWI] = 0x030;
	core->vectaddr[VECT_SPM_Ready] = 0x032;
}

void init_regs_32(struct avr_core_s* core) {
	core->regaddr[R_SREG] = 0x5F;
	core->regaddr[R_SPH] = 0x5E;
	core->regaddr[R_SPL] = 0x5D;
	core->regaddr[R_OCR0] = 0x5C;
	core->regaddr[R_GICR] = 0x5B;
	core->regaddr[R_GIFR] = 0x5A;
	core->regaddr[R_TIMSK] = 0x59;
	core->regaddr[R_TIFR] = 0x58;
	core->regaddr[R_SPMCR] = 0x57;
	core->regaddr[R_TWCR] = 0x56;
	core->regaddr[R_MCUCR] = 0x55;
	core->regaddr[R_MCUCSR] = 0x54;
	core->regaddr[R_TCCR0] = 0x53;
	core->regaddr[R_TCNT0] = 0x52;
	core->regaddr[R_OSCCAL] = 0x51;
	core->regaddr[R_OCDR] = 0x51;
	core->regaddr[R_SFIOR] = 0x50;
	core->regaddr[R_TCCR1A] = 0x4F;
	core->regaddr[R_TCCR1B] = 0x4E;
	core->regaddr[R_TCNT1H] = 0x4D;
	core->regaddr[R_TCNT1L] = 0x4C;
	core->regaddr[R_OCR1AH] = 0x4B;
	core->regaddr[R_OCR1AL] = 0x4A;
	core->regaddr[R_OCR1BH] = 0x49;
	core->regaddr[R_OCR1BL] = 0x48;
	core->regaddr[R_ICR1H] = 0x47;
	core->regaddr[R_ICR1L] = 0x46;
	core->regaddr[R_TCCR2] = 0x45;
	core->regaddr[R_TCNT2] = 0x44;
	core->regaddr[R_OCR2] = 0x43;
	core->regaddr[R_ASSR] = 0x42;
	core->regaddr[R_WDTCR] = 0x41;
	core->regaddr[R_UBRRH] = 0x40;
	core->regaddr[R_UCSRC] = 0x40;
	core->regaddr[R_EEARH] = 0x3F;
	core->regaddr[R_EEARL] = 0x3E;
	core->regaddr[R_EEDR] = 0x3D;
	core->regaddr[R_EECR] = 0x3C;
	core->regaddr[R_PORTA] = 0x3B;
	core->regaddr[R_DDRA] = 0x3A;
	core->regaddr[R_PINA] = 0x39;
	core->regaddr[R_PORTB] = 0x38;
	core->regaddr[R_DDRB] = 0x37;
	core->regaddr[R_PINB] = 0x36;
	core->regaddr[R_PORTC] = 0x35;
	core->regaddr[R_DDRC] = 0x34;
	core->regaddr[R_PINC] = 0x33;
	core->regaddr[R_PORTD] = 0x32;
	core->regaddr[R_DDRD] = 0x31;
	core->regaddr[R_PIND] = 0x30;
	core->regaddr[R_SPDR] = 0x2F;
	core->regaddr[R_SPSR] = 0x2E;
	core->regaddr[R_SPCR] = 0x2D;
	core->regaddr[R_UDR] = 0x2C;
	core->regaddr[R_UCSRA] = 0x2B;
	core->regaddr[R_UCSRB] = 0x2A;
	core->regaddr[R_UBRRL] = 0x29;
	core->regaddr[R_ACSR] = 0x28;
	core->regaddr[R_ADMUX] = 0x27;
	core->regaddr[R_ADCSRA] = 0x26;
	core->regaddr[R_ADCH] = 0x25;
	core->regaddr[R_ADCL] = 0x24;
	core->regaddr[R_TWDR] = 0x23;
	core->regaddr[R_TWAR] = 0x22;
	core->regaddr[R_TWSR] = 0x21;
	core->regaddr[R_TWBR] = 0x20;

	core->vectaddr[VECT_RESET] = 0x0;
	core->vectaddr[VECT_INT0] = 0x2;
	core->vectaddr[VECT_INT1] = 0x4;
	core->vectaddr[VECT_INT2] = 0x6;
	core->vectaddr[VECT_TIMER2_COMP] = 0x8;
	core->vectaddr[VECT_TIMER2_OVF] = 0xA;
	core->vectaddr[VECT_TIMER1_CAPT] = 0xC;
	core->vectaddr[VECT_TIMER1_COMPA] = 0xE;
	core->vectaddr[VECT_TIMER1_COMPB] = 0x10;
	core->vectaddr[VECT_TIMER1_OVF] = 0x12;
	core->vectaddr[VECT_TIMER0_COMP] = 0x14;
	core->vectaddr[VECT_TIMER0_OVF] = 0x16;
	core->vectaddr[VECT_SPI_STC] = 0x18;
	core->vectaddr[VECT_USART_RXC] = 0x1A;
	core->vectaddr[VECT_USART_UDRE] = 0x1C;
	core->vectaddr[VECT_USART_TXC] = 0x1E;
	core->vectaddr[VECT_ADC] = 0x20;
	core->vectaddr[VECT_EE_RDY] = 0x22;
	core->vectaddr[VECT_ANA_COMP] = 0x24;
	core->vectaddr[VECT_TWI] = 0x26;
	core->vectaddr[VECT_SPM_RDY] = 0x28;

	//EEPROM
	core->io_write[REG(R_EEARH)] = eeprom_write; core->io_read[REG(R_EEARH)] = eeprom_read;
	core->io_write[REG(R_EEARL)] = eeprom_write; core->io_read[REG(R_EEARL)] = eeprom_read;
	core->io_write[REG(R_EEDR)] = eeprom_write; core->io_read[REG(R_EEDR)] = eeprom_read;
	core->io_write[REG(R_EECR)] = eeprom_write; core->io_read[REG(R_EECR)] = eeprom_read;
	periph_add_clock(NULL, eeprom_clock);

	//TIMER1
	timer16_init(&timer16_1);
	timer16_1.int_vector_OCA = core->vectaddr[VECT_TIMER1_COMPA];
	timer16_1.int_vector_OCB = core->vectaddr[VECT_TIMER1_COMPB];
	timer16_1.int_vector_OCC = core->vectaddr[VECT_TIMER1_COMPC];
	timer16_1.int_vector_OVF = core->vectaddr[VECT_TIMER1_OVF];
	timer16_1.addr_ocrbl = REG(R_OCR1BL);
	timer16_1.addr_ocrbh = REG(R_OCR1BH);
	timer16_1.addr_ocral = REG(R_OCR1AL);
	timer16_1.addr_ocrah = REG(R_OCR1AH);
	timer16_1.addr_ocrbl = REG(R_ICR1L);
	timer16_1.addr_ocrbh = REG(R_ICR1H);
	timer16_1.addr_tccra = REG(R_TCCR1A);
	timer16_1.addr_tccrb = REG(R_TCCR1B);
	timer16_1.addr_tcntl = REG(R_TCNT1L);
	timer16_1.addr_tcnth = REG(R_TCNT1H);
	timer16_1.addr_timsk = REG(R_TIMSK);
	timer16_1.addr_tifr = REG(R_TIFR);
	timer16_1.bit_mask_ocra = 4;
	timer16_1.bit_mask_ovf = 2;
	core->io_write[REG(R_OCR1BL)] = timer16_write; core->io_read[REG(R_OCR1BL)] = timer16_read; core->peripheral[REG(R_OCR1BL)] = &timer16_1;
	core->io_write[REG(R_OCR1BH)] = timer16_write; core->io_read[REG(R_OCR1BH)] = timer16_read; core->peripheral[REG(R_OCR1BH)] = &timer16_1;
	core->io_write[REG(R_OCR1AL)] = timer16_write; core->io_read[REG(R_OCR1AL)] = timer16_read; core->peripheral[REG(R_OCR1AL)] = &timer16_1;
	core->io_write[REG(R_OCR1AH)] = timer16_write; core->io_read[REG(R_OCR1AH)] = timer16_read; core->peripheral[REG(R_OCR1AH)] = &timer16_1;
	core->io_write[REG(R_ICR1L)] = timer16_write; core->io_read[REG(R_ICR1L)] = timer16_read; core->peripheral[REG(R_ICR1L)] = &timer16_1;
	core->io_write[REG(R_ICR1H)] = timer16_write; core->io_read[REG(R_ICR1H)] = timer16_read; core->peripheral[REG(R_ICR1H)] = &timer16_1;
	core->io_write[REG(R_TCCR1A)] = timer16_write; core->io_read[REG(R_TCCR1A)] = timer16_read; core->peripheral[REG(R_TCCR1A)] = &timer16_1;
	core->io_write[REG(R_TCCR1B)] = timer16_write; core->io_read[REG(R_TCCR1B)] = timer16_read; core->peripheral[REG(R_TCCR1B)] = &timer16_1;
	core->io_write[REG(R_TCNT1L)] = timer16_write; core->io_read[REG(R_TCNT1L)] = timer16_read; core->peripheral[REG(R_TCNT1L)] = &timer16_1;
	core->io_write[REG(R_TCNT1H)] = timer16_write; core->io_read[REG(R_TCNT1H)] = timer16_read; core->peripheral[REG(R_TCNT1H)] = &timer16_1;
	core->io_write[REG(R_TIMSK)] = timer16_write; core->io_read[REG(R_TIMSK)] = timer16_read; core->peripheral[REG(R_TIMSK)] = &timer16_1;
	core->io_write[REG(R_TIFR)] = timer16_write; core->io_read[REG(R_TIFR)] = timer16_read; core->peripheral[REG(R_TIFR)] = &timer16_1;
	periph_add_clock(&timer16_1, timer16_clock);
}

void init_regs_64(struct avr_core_s* core) {
	core->regaddr[R_UCSR1C] = 0x9D;
	core->regaddr[R_UDR1] = 0x9C;
	core->regaddr[R_UCSR1A] = 0x9B;
	core->regaddr[R_UCSR1B] = 0x9A;
	core->regaddr[R_UBRR1L] = 0x99;
	core->regaddr[R_UBRR1H] = 0x98;
	core->regaddr[R_UCSR0C] = 0x95;
	core->regaddr[R_UBRR0H] = 0x90;
	core->regaddr[R_ADCSRB] = 0x8E;
	core->regaddr[R_TCCR3C] = 0x8C;
	core->regaddr[R_TCCR3A] = 0x8B;
	core->regaddr[R_TCCR3B] = 0x8A;
	core->regaddr[R_TCNT3H] = 0x89;
	core->regaddr[R_TCNT3L] = 0x88;
	core->regaddr[R_OCR3AH] = 0x87;
	core->regaddr[R_OCR3AL] = 0x86;
	core->regaddr[R_OCR3BH] = 0x85;
	core->regaddr[R_OCR3BL] = 0x84;
	core->regaddr[R_OCR3CH] = 0x83;
	core->regaddr[R_OCR3CL] = 0x82;
	core->regaddr[R_ICR3H] = 0x81;
	core->regaddr[R_ICR3L] = 0x80;
	core->regaddr[R_ETIMSK] = 0x7D;
	core->regaddr[R_ETIFR] = 0x7C;
	core->regaddr[R_TCCR1C] = 0x7A;
	core->regaddr[R_OCR1CH] = 0x79;
	core->regaddr[R_OCR1CL] = 0x78;
	core->regaddr[R_TWCR] = 0x74;
	core->regaddr[R_TWDR] = 0x73;
	core->regaddr[R_TWAR] = 0x72;
	core->regaddr[R_TWSR] = 0x71;
	core->regaddr[R_TWBR] = 0x70;
	core->regaddr[R_OSCCAL] = 0x6F;
	core->regaddr[R_XMCRA] = 0x6D;
	core->regaddr[R_XMCRB] = 0x6C;
	core->regaddr[R_EICRA] = 0x6A;
	core->regaddr[R_SPMCSR] = 0x68;
	core->regaddr[R_PORTG] = 0x65;
	core->regaddr[R_DDRG] = 0x64;
	core->regaddr[R_PING] = 0x63;
	core->regaddr[R_PORTF] = 0x62;
	core->regaddr[R_DDRF] = 0x61;
	core->regaddr[R_SREG] = 0x5F;
	core->regaddr[R_SPH] = 0x5E;
	core->regaddr[R_SPL] = 0x5D;
	core->regaddr[R_XDIV] = 0x5C;
	core->regaddr[R_EICRB] = 0x5A;
	core->regaddr[R_EIMSK] = 0x59;
	core->regaddr[R_EIFR] = 0x58;
	core->regaddr[R_TIMSK] = 0x57;
	core->regaddr[R_TIFR] = 0x56;
	core->regaddr[R_MCUCR] = 0x55;
	core->regaddr[R_MCUCSR] = 0x54;
	core->regaddr[R_TCCR0] = 0x53;
	core->regaddr[R_TCNT0] = 0x52;
	core->regaddr[R_OCR0] = 0x51;
	core->regaddr[R_ASSR] = 0x50;
	core->regaddr[R_TCCR1A] = 0x4F;
	core->regaddr[R_TCCR1B] = 0x4E;
	core->regaddr[R_TCNT1H] = 0x4D;
	core->regaddr[R_TCNT1L] = 0x4C;
	core->regaddr[R_OCR1AH] = 0x4B;
	core->regaddr[R_OCR1AL] = 0x4A;
	core->regaddr[R_OCR1BH] = 0x49;
	core->regaddr[R_OCR1BL] = 0x48;
	core->regaddr[R_ICR1H] = 0x47;
	core->regaddr[R_ICR1L] = 0x46;
	core->regaddr[R_TCCR2] = 0x45;
	core->regaddr[R_TCNT2] = 0x44;
	core->regaddr[R_OCR2] = 0x43;
	core->regaddr[R_OCDR] = 0x42;
	core->regaddr[R_WDTCR] = 0x41;
	core->regaddr[R_SFIOR] = 0x40;
	core->regaddr[R_EEARH] = 0x3F;
	core->regaddr[R_EEARL] = 0x3E;
	core->regaddr[R_EEDR] = 0x3D;
	core->regaddr[R_EECR] = 0x3C;
	core->regaddr[R_PORTA] = 0x3B;
	core->regaddr[R_DDRA] = 0x3A;
	core->regaddr[R_PINA] = 0x39;
	core->regaddr[R_PORTB] = 0x38;
	core->regaddr[R_DDRB] = 0x37;
	core->regaddr[R_PINB] = 0x36;
	core->regaddr[R_PORTC] = 0x35;
	core->regaddr[R_DDRC] = 0x34;
	core->regaddr[R_PINC] = 0x33;
	core->regaddr[R_PORTD] = 0x32;
	core->regaddr[R_DDRD] = 0x31;
	core->regaddr[R_PIND] = 0x30;
	core->regaddr[R_SPDR] = 0x2F;
	core->regaddr[R_SPSR] = 0x2E;
	core->regaddr[R_SPCR] = 0x2D;
	core->regaddr[R_UDR0] = 0x2C;
	core->regaddr[R_UCSR0A] = 0x2B;
	core->regaddr[R_UCSR0B] = 0x2A;
	core->regaddr[R_UBRR0L] = 0x29;
	core->regaddr[R_ACSR] = 0x28;
	core->regaddr[R_ADMUX] = 0x27;
	core->regaddr[R_ADCSRA] = 0x26;
	core->regaddr[R_ADCH] = 0x25;
	core->regaddr[R_ADCL] = 0x24;
	core->regaddr[R_PORTE] = 0x23;
	core->regaddr[R_DDRE] = 0x22;
	core->regaddr[R_PINE] = 0x21;
	core->regaddr[R_PINF] = 0x20;

	core->vectaddr[VECT_RESET] = 0x0;
	core->vectaddr[VECT_INT0] = 0x2;
	core->vectaddr[VECT_INT1] = 0x4;
	core->vectaddr[VECT_INT2] = 0x6;
	core->vectaddr[VECT_INT3] = 0x8;
	core->vectaddr[VECT_INT4] = 0xA;
	core->vectaddr[VECT_INT5] = 0xC;
	core->vectaddr[VECT_INT6] = 0xE;
	core->vectaddr[VECT_INT7] = 0x10;
	core->vectaddr[VECT_TIMER2_COMP] = 0x12;
	core->vectaddr[VECT_TIMER2_OVF] = 0x14;
	core->vectaddr[VECT_TIMER1_CAPT] = 0x16;
	core->vectaddr[VECT_TIMER1_COMPA] = 0x18;
	core->vectaddr[VECT_TIMER1_COMPB] = 0x1A;
	core->vectaddr[VECT_TIMER1_OVF] = 0x1C;
	core->vectaddr[VECT_TIMER0_COMP] = 0x1E;
	core->vectaddr[VECT_TIMER0_OVF] = 0x20;
	core->vectaddr[VECT_SPI_STC] = 0x22;
	core->vectaddr[VECT_USART0_RXC] = 0x24;
	core->vectaddr[VECT_USART0_UDRE] = 0x26;
	core->vectaddr[VECT_USART0_TXC] = 0x28;
	core->vectaddr[VECT_ADC] = 0x2A;
	core->vectaddr[VECT_EE_READY] = 0x2C;
	core->vectaddr[VECT_ANALOG_COMP] = 0x2E;
	core->vectaddr[VECT_TIMER1_COMPC] = 0x30;
	core->vectaddr[VECT_TIMER3_CAPT] = 0x32;
	core->vectaddr[VECT_TIMER3_COMPA] = 0x34;
	core->vectaddr[VECT_TIMER3_COMPB] = 0x36;
	core->vectaddr[VECT_TIMER3_COMPC] = 0x38;
	core->vectaddr[VECT_TIMER3_OVF] = 0x3A;
	core->vectaddr[VECT_USART1_RXC] = 0x3C;
	core->vectaddr[VECT_USART1_UDRE] = 0x3E;
	core->vectaddr[VECT_USART1_TXC] = 0x40;
	core->vectaddr[VECT_TWI] = 0x42;
	core->vectaddr[VECT_SPM_READY] = 0x44;

	//EEPROM
	core->io_write[REG(R_EEARH)] = eeprom_write; core->io_read[REG(R_EEARH)] = eeprom_read;
	core->io_write[REG(R_EEARL)] = eeprom_write; core->io_read[REG(R_EEARL)] = eeprom_read;
	core->io_write[REG(R_EEDR)] = eeprom_write; core->io_read[REG(R_EEDR)] = eeprom_read;
	core->io_write[REG(R_EECR)] = eeprom_write; core->io_read[REG(R_EECR)] = eeprom_read;
	periph_add_clock(NULL, eeprom_clock);

	//TIMER1
	timer16_init(&timer16_1);
	timer16_1.int_vector_OCA = core->vectaddr[VECT_TIMER1_COMPA];
	timer16_1.int_vector_OCB = core->vectaddr[VECT_TIMER1_COMPB];
	timer16_1.int_vector_OCC = core->vectaddr[VECT_TIMER1_COMPC];
	timer16_1.int_vector_OVF = core->vectaddr[VECT_TIMER1_OVF];
	timer16_1.addr_ocrbl = REG(R_OCR1BL);
	timer16_1.addr_ocrbh = REG(R_OCR1BH);
	timer16_1.addr_ocral = REG(R_OCR1AL);
	timer16_1.addr_ocrah = REG(R_OCR1AH);
	timer16_1.addr_ocrbl = REG(R_ICR1L);
	timer16_1.addr_ocrbh = REG(R_ICR1H);
	timer16_1.addr_tccra = REG(R_TCCR1A);
	timer16_1.addr_tccrb = REG(R_TCCR1B);
	timer16_1.addr_tcntl = REG(R_TCNT1L);
	timer16_1.addr_tcnth = REG(R_TCNT1H);
	timer16_1.addr_timsk = REG(R_TIMSK1);
	timer16_1.addr_tifr = REG(R_TIFR1);
	timer16_1.bit_mask_ocra = 1;
	timer16_1.bit_mask_ovf = 0;
	core->io_write[REG(R_OCR1BL)] = timer16_write; core->io_read[REG(R_OCR1BL)] = timer16_read; core->peripheral[REG(R_OCR1BL)] = &timer16_1;
	core->io_write[REG(R_OCR1BH)] = timer16_write; core->io_read[REG(R_OCR1BH)] = timer16_read; core->peripheral[REG(R_OCR1BH)] = &timer16_1;
	core->io_write[REG(R_OCR1AL)] = timer16_write; core->io_read[REG(R_OCR1AL)] = timer16_read; core->peripheral[REG(R_OCR1AL)] = &timer16_1;
	core->io_write[REG(R_OCR1AH)] = timer16_write; core->io_read[REG(R_OCR1AH)] = timer16_read; core->peripheral[REG(R_OCR1AH)] = &timer16_1;
	core->io_write[REG(R_ICR1L)] = timer16_write; core->io_read[REG(R_ICR1L)] = timer16_read; core->peripheral[REG(R_ICR1L)] = &timer16_1;
	core->io_write[REG(R_ICR1H)] = timer16_write; core->io_read[REG(R_ICR1H)] = timer16_read; core->peripheral[REG(R_ICR1H)] = &timer16_1;
	core->io_write[REG(R_TCCR1A)] = timer16_write; core->io_read[REG(R_TCCR1A)] = timer16_read; core->peripheral[REG(R_TCCR1A)] = &timer16_1;
	core->io_write[REG(R_TCCR1B)] = timer16_write; core->io_read[REG(R_TCCR1B)] = timer16_read; core->peripheral[REG(R_TCCR1B)] = &timer16_1;
	core->io_write[REG(R_TCNT1L)] = timer16_write; core->io_read[REG(R_TCNT1L)] = timer16_read; core->peripheral[REG(R_TCNT1L)] = &timer16_1;
	core->io_write[REG(R_TCNT1H)] = timer16_write; core->io_read[REG(R_TCNT1H)] = timer16_read; core->peripheral[REG(R_TCNT1H)] = &timer16_1;
	core->io_write[REG(R_TIMSK1)] = timer16_write; core->io_read[REG(R_TIMSK1)] = timer16_read; core->peripheral[REG(R_TIMSK1)] = &timer16_1;
	core->io_write[REG(R_TIFR1)] = timer16_write; core->io_read[REG(R_TIFR1)] = timer16_read; core->peripheral[REG(R_TIFR1)] = &timer16_1;
	periph_add_clock(&timer16_1, timer16_clock);
}

void init_regs_640_1280_1281_2560_2561(struct avr_core_s* core) {
	core->regaddr[R_UDR3] = 0x136;
	core->regaddr[R_UBRR3H] = 0x135;
	core->regaddr[R_UBRR3L] = 0x134;
	core->regaddr[R_UCSR3C] = 0x132;
	core->regaddr[R_UCSR3B] = 0x131;
	core->regaddr[R_UCSR3A] = 0x130;
	core->regaddr[R_OCR5CH] = 0x12D;
	core->regaddr[R_OCR5CL] = 0x12C;
	core->regaddr[R_OCR5BH] = 0x12B;
	core->regaddr[R_OCR5BL] = 0x12A;
	core->regaddr[R_OCR5AH] = 0x129;
	core->regaddr[R_OCR5AL] = 0x128;
	core->regaddr[R_ICR5H] = 0x127;
	core->regaddr[R_ICR5L] = 0x126;
	core->regaddr[R_TCNT5H] = 0x125;
	core->regaddr[R_TCNT5L] = 0x124;
	core->regaddr[R_TCCR5C] = 0x122;
	core->regaddr[R_TCCR5B] = 0x121;
	core->regaddr[R_TCCR5A] = 0x120;
	core->regaddr[R_PORTL] = 0x10B;
	core->regaddr[R_DDRL] = 0x10A;
	core->regaddr[R_PINL] = 0x109;
	core->regaddr[R_PORTK] = 0x108;
	core->regaddr[R_DDRK] = 0x107;
	core->regaddr[R_PINK] = 0x106;
	core->regaddr[R_PORTJ] = 0x105;
	core->regaddr[R_DDRJ] = 0x104;
	core->regaddr[R_PINJ] = 0x103;
	core->regaddr[R_PORTH] = 0x102;
	core->regaddr[R_DDRH] = 0x101;
	core->regaddr[R_PINH] = 0x100;
	core->regaddr[R_UDR2] = 0xD6;
	core->regaddr[R_UBRR2H] = 0xD5;
	core->regaddr[R_UBRR2L] = 0xD4;
	core->regaddr[R_UCSR2C] = 0xD2;
	core->regaddr[R_UCSR2B] = 0xD1;
	core->regaddr[R_UCSR2A] = 0xD0;
	core->regaddr[R_UDR1] = 0xCE;
	core->regaddr[R_UBRR1H] = 0xCD;
	core->regaddr[R_UBRR1L] = 0xCC;
	core->regaddr[R_UCSR1C] = 0xCA;
	core->regaddr[R_UCSR1B] = 0xC9;
	core->regaddr[R_UCSR1A] = 0xC8;
	core->regaddr[R_UDR0] = 0xC6;
	core->regaddr[R_UBRR0H] = 0xC5;
	core->regaddr[R_UBRR0L] = 0xC4;
	core->regaddr[R_UCSR0C] = 0xC2;
	core->regaddr[R_UCSR0B] = 0xC1;
	core->regaddr[R_UCSR0A] = 0xC0;
	core->regaddr[R_TWAMR] = 0xBD;
	core->regaddr[R_TWCR] = 0xBC;
	core->regaddr[R_TWDR] = 0xBB;
	core->regaddr[R_TWAR] = 0xBA;
	core->regaddr[R_TWSR] = 0xB9;
	core->regaddr[R_TWBR] = 0xB8;
	core->regaddr[R_ASSR] = 0xB6;
	core->regaddr[R_OCR2B] = 0xB4;
	core->regaddr[R_OCR2A] = 0xB3;
	core->regaddr[R_TCNT2] = 0xB2;
	core->regaddr[R_TCCR2B] = 0xB1;
	core->regaddr[R_TCCR2A] = 0xB0;
	core->regaddr[R_OCR4CH] = 0xAD;
	core->regaddr[R_OCR4CL] = 0xAC;
	core->regaddr[R_OCR4BH] = 0xAB;
	core->regaddr[R_OCR4BL] = 0xAA;
	core->regaddr[R_OCR4AH] = 0xA9;
	core->regaddr[R_OCR4AL] = 0xA8;
	core->regaddr[R_ICR4H] = 0xA7;
	core->regaddr[R_ICR4L] = 0xA6;
	core->regaddr[R_TCNT4H] = 0xA5;
	core->regaddr[R_TCNT4L] = 0xA4;
	core->regaddr[R_TCCR4C] = 0xA2;
	core->regaddr[R_TCCR4B] = 0xA1;
	core->regaddr[R_TCCR4A] = 0xA0;
	core->regaddr[R_OCR3CH] = 0x9D;
	core->regaddr[R_OCR3CL] = 0x9C;
	core->regaddr[R_OCR3BH] = 0x9B;
	core->regaddr[R_OCR3BL] = 0x9A;
	core->regaddr[R_OCR3AH] = 0x99;
	core->regaddr[R_OCR3AL] = 0x98;
	core->regaddr[R_ICR3H] = 0x97;
	core->regaddr[R_ICR3L] = 0x96;
	core->regaddr[R_TCNT3H] = 0x95;
	core->regaddr[R_TCNT3L] = 0x94;
	core->regaddr[R_TCCR3C] = 0x92;
	core->regaddr[R_TCCR3B] = 0x91;
	core->regaddr[R_TCCR3A] = 0x90;
	core->regaddr[R_OCR1CH] = 0x8D;
	core->regaddr[R_OCR1CL] = 0x8C;
	core->regaddr[R_OCR1BH] = 0x8B;
	core->regaddr[R_OCR1BL] = 0x8A;
	core->regaddr[R_OCR1AH] = 0x89;
	core->regaddr[R_OCR1AL] = 0x88;
	core->regaddr[R_ICR1H] = 0x87;
	core->regaddr[R_ICR1L] = 0x86;
	core->regaddr[R_TCNT1H] = 0x85;
	core->regaddr[R_TCNT1L] = 0x84;
	core->regaddr[R_TCCR1C] = 0x82;
	core->regaddr[R_TCCR1B] = 0x81;
	core->regaddr[R_TCCR1A] = 0x80;
	core->regaddr[R_DIDR1] = 0x7F;
	core->regaddr[R_DIDR0] = 0x7E;
	core->regaddr[R_DIDR2] = 0x7D;
	core->regaddr[R_ADMUX] = 0x7C;
	core->regaddr[R_ADCSRB] = 0x7B;
	core->regaddr[R_ADCSRA] = 0x7A;
	core->regaddr[R_ADCH] = 0x79;
	core->regaddr[R_ADCL] = 0x78;
	core->regaddr[R_XMCRB] = 0x75;
	core->regaddr[R_XMCRA] = 0x74;
	core->regaddr[R_TIMSK5] = 0x73;
	core->regaddr[R_TIMSK4] = 0x72;
	core->regaddr[R_TIMSK3] = 0x71;
	core->regaddr[R_TIMSK2] = 0x70;
	core->regaddr[R_TIMSK1] = 0x6F;
	core->regaddr[R_TIMSK0] = 0x6E;
	core->regaddr[R_PCMSK2] = 0x6D;
	core->regaddr[R_PCMSK1] = 0x6C;
	core->regaddr[R_PCMSK0] = 0x6B;
	core->regaddr[R_EICRB] = 0x6A;
	core->regaddr[R_EICRA] = 0x69;
	core->regaddr[R_PCICR] = 0x68;
	core->regaddr[R_OSCCAL] = 0x66;
	core->regaddr[R_PRR1] = 0x65;
	core->regaddr[R_PRR0] = 0x64;
	core->regaddr[R_CLKPR] = 0x61;
	core->regaddr[R_WDTCSR] = 0x60;
	core->regaddr[R_SREG] = 0x5F;
	core->regaddr[R_SPH] = 0x5E;
	core->regaddr[R_SPL] = 0x5D;
	core->regaddr[R_EIND] = 0x5C;
	core->regaddr[R_RAMPZ] = 0x5B;
	core->regaddr[R_SPMCSR] = 0x57;
	core->regaddr[R_MCUCR] = 0x55;
	core->regaddr[R_MCUSR] = 0x54;
	core->regaddr[R_SMCR] = 0x53;
	core->regaddr[R_OCDR] = 0x51;
	core->regaddr[R_ACSR] = 0x50;
	core->regaddr[R_SPDR] = 0x4E;
	core->regaddr[R_SPSR] = 0x4D;
	core->regaddr[R_SPCR] = 0x4C;
	core->regaddr[R_GPIOR2] = 0x4B;
	core->regaddr[R_GPIOR1] = 0x4A;
	core->regaddr[R_OCR0B] = 0x48;
	core->regaddr[R_OCR0A] = 0x47;
	core->regaddr[R_TCNT0] = 0x46;
	core->regaddr[R_TCCR0B] = 0x45;
	core->regaddr[R_TCCR0A] = 0x44;
	core->regaddr[R_GTCCR] = 0x43;
	core->regaddr[R_EEARH] = 0x42;
	core->regaddr[R_EEARL] = 0x41;
	core->regaddr[R_EEDR] = 0x40;
	core->regaddr[R_EECR] = 0x3F;
	core->regaddr[R_GPIOR0] = 0x3E;
	core->regaddr[R_EIMSK] = 0x3D;
	core->regaddr[R_EIFR] = 0x3C;
	core->regaddr[R_PCIFR] = 0x3B;
	core->regaddr[R_TIFR5] = 0x3A;
	core->regaddr[R_TIFR4] = 0x39;
	core->regaddr[R_TIFR3] = 0x38;
	core->regaddr[R_TIFR2] = 0x37;
	core->regaddr[R_TIFR1] = 0x36;
	core->regaddr[R_TIFR0] = 0x35;
	core->regaddr[R_PORTG] = 0x34;
	core->regaddr[R_DDRG] = 0x33;
	core->regaddr[R_PING] = 0x32;
	core->regaddr[R_PORTF] = 0x31;
	core->regaddr[R_DDRF] = 0x30;
	core->regaddr[R_PINF] = 0x2F;
	core->regaddr[R_PORTE] = 0x2E;
	core->regaddr[R_DDRE] = 0x2D;
	core->regaddr[R_PINE] = 0x2C;
	core->regaddr[R_PORTD] = 0x2B;
	core->regaddr[R_DDRD] = 0x2A;
	core->regaddr[R_PIND] = 0x29;
	core->regaddr[R_PORTC] = 0x28;
	core->regaddr[R_DDRC] = 0x27;
	core->regaddr[R_PINC] = 0x26;
	core->regaddr[R_PORTB] = 0x25;
	core->regaddr[R_DDRB] = 0x24;
	core->regaddr[R_PINB] = 0x23;
	core->regaddr[R_PORTA] = 0x22;
	core->regaddr[R_DDRA] = 0x21;
	core->regaddr[R_PINA] = 0x20;

	core->vectaddr[VECT_RESET] = 0x0;
	core->vectaddr[VECT_INT0] = 0x2;
	core->vectaddr[VECT_INT1] = 0x4;
	core->vectaddr[VECT_INT2] = 0x6;
	core->vectaddr[VECT_INT3] = 0x8;
	core->vectaddr[VECT_INT4] = 0xA;
	core->vectaddr[VECT_INT5] = 0xC;
	core->vectaddr[VECT_INT6] = 0xE;
	core->vectaddr[VECT_INT7] = 0x10;
	core->vectaddr[VECT_PCINT0] = 0x12;
	core->vectaddr[VECT_PCINT1] = 0x14;
	core->vectaddr[VECT_PCINT2] = 0x16;
	core->vectaddr[VECT_WDT] = 0x18;
	core->vectaddr[VECT_TIMER2_COMPA] = 0x1A;
	core->vectaddr[VECT_TIMER2_COMPB] = 0x1C;
	core->vectaddr[VECT_TIMER2_OVF] = 0x1E;
	core->vectaddr[VECT_TIMER1_CAPT] = 0x20;
	core->vectaddr[VECT_TIMER1_COMPA] = 0x22;
	core->vectaddr[VECT_TIMER1_COMPB] = 0x24;
	core->vectaddr[VECT_TIMER1_COMPC] = 0x26;
	core->vectaddr[VECT_TIMER1_OVF] = 0x28;
	core->vectaddr[VECT_TIMER0_COMPA] = 0x2A;
	core->vectaddr[VECT_TIMER0_COMPB] = 0x2C;
	core->vectaddr[VECT_TIMER0_OVF] = 0x2E;
	core->vectaddr[VECT_SPI_STC] = 0x30;
	core->vectaddr[VECT_USART0_RXC] = 0x32;
	core->vectaddr[VECT_USART0_UDRE] = 0x34;
	core->vectaddr[VECT_USART0_TXC] = 0x36;
	core->vectaddr[VECT_ANALOG_COMP] = 0x38;
	core->vectaddr[VECT_ADC] = 0x3A;
	core->vectaddr[VECT_EE_READY] = 0x3C;
	core->vectaddr[VECT_TIMER3_CAPT] = 0x3E;
	core->vectaddr[VECT_TIMER3_COMPA] = 0x40;
	core->vectaddr[VECT_TIMER3_COMPB] = 0x42;
	core->vectaddr[VECT_TIMER3_COMPC] = 0x44;
	core->vectaddr[VECT_TIMER3_OVF] = 0x46;
	core->vectaddr[VECT_USART1_RXC] = 0x48;
	core->vectaddr[VECT_USART1_UDRE] = 0x4A;
	core->vectaddr[VECT_USART1_TXC] = 0x4C;
	core->vectaddr[VECT_TWI] = 0x4E;
	core->vectaddr[VECT_SPM_READY] = 0x50;
	core->vectaddr[VECT_TIMER4_CAPT] = 0x52;
	core->vectaddr[VECT_TIMER4_COMPA] = 0x54;
	core->vectaddr[VECT_TIMER4_COMPB] = 0x56;
	core->vectaddr[VECT_TIMER4_COMPC] = 0x58;
	core->vectaddr[VECT_TIMER4_OVF] = 0x5A;
	core->vectaddr[VECT_TIMER5_CAPT] = 0x5C;
	core->vectaddr[VECT_TIMER5_COMPA] = 0x5E;
	core->vectaddr[VECT_TIMER5_COMPB] = 0x60;
	core->vectaddr[VECT_TIMER5_COMPC] = 0x62;
	core->vectaddr[VECT_TIMER5_OVF] = 0x64;
	core->vectaddr[VECT_USART2_RXC] = 0x66;
	core->vectaddr[VECT_USART2_UDRE] = 0x68;
	core->vectaddr[VECT_USART2_TXC] = 0x6A;
	core->vectaddr[VECT_USART3_RXC] = 0x6C;
	core->vectaddr[VECT_USART3_UDRE] = 0x6E;
	core->vectaddr[VECT_USART3_TXC] = 0x70;

	core->xmem_size = 65536UL;

	//EEPROM
	core->io_write[REG(R_EEARH)] = eeprom_write; core->io_read[REG(R_EEARH)] = eeprom_read;
	core->io_write[REG(R_EEARL)] = eeprom_write; core->io_read[REG(R_EEARL)] = eeprom_read;
	core->io_write[REG(R_EEDR)] = eeprom_write; core->io_read[REG(R_EEDR)] = eeprom_read;
	core->io_write[REG(R_EECR)] = eeprom_write; core->io_read[REG(R_EECR)] = eeprom_read;
	periph_add_clock(NULL, eeprom_clock);

	//TIMER1
	timer16_init(&timer16_1);
	timer16_1.int_vector_OCA = core->vectaddr[VECT_TIMER1_COMPA];
	timer16_1.int_vector_OCB = core->vectaddr[VECT_TIMER1_COMPB];
	timer16_1.int_vector_OCC = core->vectaddr[VECT_TIMER1_COMPC];
	timer16_1.int_vector_OVF = core->vectaddr[VECT_TIMER1_OVF];
	timer16_1.addr_ocrcl = REG(R_OCR1CL);
	timer16_1.addr_ocrch = REG(R_OCR1CH);
	timer16_1.addr_ocrbl = REG(R_OCR1BL);
	timer16_1.addr_ocrbh = REG(R_OCR1BH);
	timer16_1.addr_ocral = REG(R_OCR1AL);
	timer16_1.addr_ocrah = REG(R_OCR1AH);
	timer16_1.addr_ocrbl = REG(R_ICR1L);
	timer16_1.addr_ocrbh = REG(R_ICR1H);
	timer16_1.addr_tccra = REG(R_TCCR1A);
	timer16_1.addr_tccrb = REG(R_TCCR1B);
	timer16_1.addr_tcntl = REG(R_TCNT1L);
	timer16_1.addr_tcnth = REG(R_TCNT1H);
	timer16_1.addr_timsk = REG(R_TIMSK1);
	timer16_1.addr_tifr = REG(R_TIFR1);
	timer16_1.bit_mask_ocra = 1;
	timer16_1.bit_mask_ovf = 0;
	core->io_write[REG(R_OCR1CL)] = timer16_write; core->io_read[REG(R_OCR1CL)] = timer16_read; core->peripheral[REG(R_OCR1CL)] = &timer16_1;
	core->io_write[REG(R_OCR1CH)] = timer16_write; core->io_read[REG(R_OCR1CH)] = timer16_read; core->peripheral[REG(R_OCR1CH)] = &timer16_1;
	core->io_write[REG(R_OCR1BL)] = timer16_write; core->io_read[REG(R_OCR1BL)] = timer16_read; core->peripheral[REG(R_OCR1BL)] = &timer16_1;
	core->io_write[REG(R_OCR1BH)] = timer16_write; core->io_read[REG(R_OCR1BH)] = timer16_read; core->peripheral[REG(R_OCR1BH)] = &timer16_1;
	core->io_write[REG(R_OCR1AL)] = timer16_write; core->io_read[REG(R_OCR1AL)] = timer16_read; core->peripheral[REG(R_OCR1AL)] = &timer16_1;
	core->io_write[REG(R_OCR1AH)] = timer16_write; core->io_read[REG(R_OCR1AH)] = timer16_read; core->peripheral[REG(R_OCR1AH)] = &timer16_1;
	core->io_write[REG(R_ICR1L)] = timer16_write; core->io_read[REG(R_ICR1L)] = timer16_read; core->peripheral[REG(R_ICR1L)] = &timer16_1;
	core->io_write[REG(R_ICR1H)] = timer16_write; core->io_read[REG(R_ICR1H)] = timer16_read; core->peripheral[REG(R_ICR1H)] = &timer16_1;
	core->io_write[REG(R_TCCR1A)] = timer16_write; core->io_read[REG(R_TCCR1A)] = timer16_read; core->peripheral[REG(R_TCCR1A)] = &timer16_1;
	core->io_write[REG(R_TCCR1B)] = timer16_write; core->io_read[REG(R_TCCR1B)] = timer16_read; core->peripheral[REG(R_TCCR1B)] = &timer16_1;
	core->io_write[REG(R_TCNT1L)] = timer16_write; core->io_read[REG(R_TCNT1L)] = timer16_read; core->peripheral[REG(R_TCNT1L)] = &timer16_1;
	core->io_write[REG(R_TCNT1H)] = timer16_write; core->io_read[REG(R_TCNT1H)] = timer16_read; core->peripheral[REG(R_TCNT1H)] = &timer16_1;
	core->io_write[REG(R_TIMSK1)] = timer16_write; core->io_read[REG(R_TIMSK1)] = timer16_read; core->peripheral[REG(R_TIMSK1)] = &timer16_1;
	core->io_write[REG(R_TIFR1)] = timer16_write; core->io_read[REG(R_TIFR1)] = timer16_read; core->peripheral[REG(R_TIFR1)] = &timer16_1;
	periph_add_clock(&timer16_1, timer16_clock);

	//TIMER3
	timer16_init(&timer16_3);
	timer16_3.int_vector_OCA = core->vectaddr[VECT_TIMER3_COMPA];
	timer16_3.int_vector_OCB = core->vectaddr[VECT_TIMER3_COMPB];
	timer16_3.int_vector_OCC = core->vectaddr[VECT_TIMER3_COMPC];
	timer16_3.int_vector_OVF = core->vectaddr[VECT_TIMER3_OVF];
	timer16_3.addr_ocrcl = REG(R_OCR3CL);
	timer16_3.addr_ocrch = REG(R_OCR3CH);
	timer16_3.addr_ocrbl = REG(R_OCR3BL);
	timer16_3.addr_ocrbh = REG(R_OCR3BH);
	timer16_3.addr_ocral = REG(R_OCR3AL);
	timer16_3.addr_ocrah = REG(R_OCR3AH);
	timer16_3.addr_ocrbl = REG(R_ICR3L);
	timer16_3.addr_ocrbh = REG(R_ICR3H);
	timer16_3.addr_tccra = REG(R_TCCR3A);
	timer16_3.addr_tccrb = REG(R_TCCR3B);
	timer16_3.addr_tcntl = REG(R_TCNT3L);
	timer16_3.addr_tcnth = REG(R_TCNT3H);
	timer16_3.addr_timsk = REG(R_TIMSK3);
	timer16_3.addr_tifr = REG(R_TIFR3);
	timer16_3.bit_mask_ocra = 1;
	timer16_3.bit_mask_ovf = 0;
	core->io_write[REG(R_OCR3CL)] = timer16_write; core->io_read[REG(R_OCR3CL)] = timer16_read; core->peripheral[REG(R_OCR3CL)] = &timer16_3;
	core->io_write[REG(R_OCR3CH)] = timer16_write; core->io_read[REG(R_OCR3CH)] = timer16_read; core->peripheral[REG(R_OCR3CH)] = &timer16_3;
	core->io_write[REG(R_OCR3BL)] = timer16_write; core->io_read[REG(R_OCR3BL)] = timer16_read; core->peripheral[REG(R_OCR3BL)] = &timer16_3;
	core->io_write[REG(R_OCR3BH)] = timer16_write; core->io_read[REG(R_OCR3BH)] = timer16_read; core->peripheral[REG(R_OCR3BH)] = &timer16_3;
	core->io_write[REG(R_OCR3AL)] = timer16_write; core->io_read[REG(R_OCR3AL)] = timer16_read; core->peripheral[REG(R_OCR3AL)] = &timer16_3;
	core->io_write[REG(R_OCR3AH)] = timer16_write; core->io_read[REG(R_OCR3AH)] = timer16_read; core->peripheral[REG(R_OCR3AH)] = &timer16_3;
	core->io_write[REG(R_ICR3L)] = timer16_write; core->io_read[REG(R_ICR3L)] = timer16_read; core->peripheral[REG(R_ICR3L)] = &timer16_3;
	core->io_write[REG(R_ICR3H)] = timer16_write; core->io_read[REG(R_ICR3H)] = timer16_read; core->peripheral[REG(R_ICR3H)] = &timer16_3;
	core->io_write[REG(R_TCCR3A)] = timer16_write; core->io_read[REG(R_TCCR3A)] = timer16_read; core->peripheral[REG(R_TCCR3A)] = &timer16_3;
	core->io_write[REG(R_TCCR3B)] = timer16_write; core->io_read[REG(R_TCCR3B)] = timer16_read; core->peripheral[REG(R_TCCR3B)] = &timer16_3;
	core->io_write[REG(R_TCNT3L)] = timer16_write; core->io_read[REG(R_TCNT3L)] = timer16_read; core->peripheral[REG(R_TCNT3L)] = &timer16_3;
	core->io_write[REG(R_TCNT3H)] = timer16_write; core->io_read[REG(R_TCNT3H)] = timer16_read; core->peripheral[REG(R_TCNT3H)] = &timer16_3;
	core->io_write[REG(R_TIMSK3)] = timer16_write; core->io_read[REG(R_TIMSK3)] = timer16_read; core->peripheral[REG(R_TIMSK3)] = &timer16_3;
	core->io_write[REG(R_TIFR3)] = timer16_write; core->io_read[REG(R_TIFR3)] = timer16_read; core->peripheral[REG(R_TIFR3)] = &timer16_3;
	periph_add_clock(&timer16_3, timer16_clock);

}

void init_regs_8(struct avr_core_s* core) {
	core->regaddr[R_SREG] = 0x5F;
	core->regaddr[R_SPH] = 0x5E;
	core->regaddr[R_SPL] = 0x5D;
	core->regaddr[R_GICR] = 0x5B;
	core->regaddr[R_GIFR] = 0x5A;
	core->regaddr[R_TIMSK] = 0x59;
	core->regaddr[R_TIFR] = 0x58;
	core->regaddr[R_SPMCR] = 0x57;
	core->regaddr[R_TWCR] = 0x56;
	core->regaddr[R_MCUCR] = 0x55;
	core->regaddr[R_MCUCSR] = 0x54;
	core->regaddr[R_TCCR0] = 0x53;
	core->regaddr[R_TCNT0] = 0x52;
	core->regaddr[R_OSCCAL] = 0x51;
	core->regaddr[R_SFIOR] = 0x50;
	core->regaddr[R_TCCR1A] = 0x4F;
	core->regaddr[R_TCCR1B] = 0x4E;
	core->regaddr[R_TCNT1H] = 0x4D;
	core->regaddr[R_TCNT1L] = 0x4C;
	core->regaddr[R_OCR1AH] = 0x4B;
	core->regaddr[R_OCR1AL] = 0x4A;
	core->regaddr[R_OCR1BH] = 0x49;
	core->regaddr[R_OCR1BL] = 0x48;
	core->regaddr[R_ICR1H] = 0x47;
	core->regaddr[R_ICR1L] = 0x46;
	core->regaddr[R_TCCR2] = 0x45;
	core->regaddr[R_TCNT2] = 0x44;
	core->regaddr[R_OCR2] = 0x43;
	core->regaddr[R_ASSR] = 0x42;
	core->regaddr[R_WDTCR] = 0x41;
	core->regaddr[R_UBRRH] = 0x40;
	core->regaddr[R_UCSRC] = 0x40;
	core->regaddr[R_EEARH] = 0x3F;
	core->regaddr[R_EEARL] = 0x3E;
	core->regaddr[R_EEDR] = 0x3D;
	core->regaddr[R_EECR] = 0x3C;
	core->regaddr[R_PORTB] = 0x38;
	core->regaddr[R_DDRB] = 0x37;
	core->regaddr[R_PINB] = 0x36;
	core->regaddr[R_PORTC] = 0x35;
	core->regaddr[R_DDRC] = 0x34;
	core->regaddr[R_PINC] = 0x33;
	core->regaddr[R_PORTD] = 0x32;
	core->regaddr[R_DDRD] = 0x31;
	core->regaddr[R_PIND] = 0x30;
	core->regaddr[R_SPDR] = 0x2F;
	core->regaddr[R_SPSR] = 0x2E;
	core->regaddr[R_SPCR] = 0x2D;
	core->regaddr[R_UDR] = 0x2C;
	core->regaddr[R_UCSRA] = 0x2B;
	core->regaddr[R_UCSRB] = 0x2A;
	core->regaddr[R_UBRRL] = 0x29;
	core->regaddr[R_ACSR] = 0x28;
	core->regaddr[R_ADMUX] = 0x27;
	core->regaddr[R_ADCSRA] = 0x26;
	core->regaddr[R_ADCH] = 0x25;
	core->regaddr[R_ADCL] = 0x24;
	core->regaddr[R_TWDR] = 0x23;
	core->regaddr[R_TWAR] = 0x22;
	core->regaddr[R_TWSR] = 0x21;
	core->regaddr[R_TWBR] = 0x20;

	core->vectaddr[VECT_RESET] = 0x0;
	core->vectaddr[VECT_INT0] = 0x1;
	core->vectaddr[VECT_INT1] = 0x2;
	core->vectaddr[VECT_TIMER2_COMP] = 0x3;
	core->vectaddr[VECT_TIMER2_OVF] = 0x4;
	core->vectaddr[VECT_TIMER1_CAPT] = 0x5;
	core->vectaddr[VECT_TIMER1_COMPA] = 0x6;
	core->vectaddr[VECT_TIMER1_COMPB] = 0x7;
	core->vectaddr[VECT_TIMER1_OVF] = 0x8;
	core->vectaddr[VECT_TIMER0_OVF] = 0x9;
	core->vectaddr[VECT_SPI_STC] = 0xA;
	core->vectaddr[VECT_USART_RXC] = 0xB;
	core->vectaddr[VECT_USART_UDRE] = 0xC;
	core->vectaddr[VECT_USART_TXC] = 0xD;
	core->vectaddr[VECT_ADC] = 0xE;
	core->vectaddr[VECT_EE_RDY] = 0xF;
	core->vectaddr[VECT_ANA_COMP] = 0x10;
	core->vectaddr[VECT_TWI] = 0x11;
	core->vectaddr[VECT_SPM_RDY] = 0x12;

	//EEPROM
	core->io_write[REG(R_EEARH)] = eeprom_write; core->io_read[REG(R_EEARH)] = eeprom_read;
	core->io_write[REG(R_EEARL)] = eeprom_write; core->io_read[REG(R_EEARL)] = eeprom_read;
	core->io_write[REG(R_EEDR)] = eeprom_write; core->io_read[REG(R_EEDR)] = eeprom_read;
	core->io_write[REG(R_EECR)] = eeprom_write; core->io_read[REG(R_EECR)] = eeprom_read;
	periph_add_clock(NULL, eeprom_clock);

	//TIMER1
	timer16_init(&timer16_1);
	timer16_1.int_vector_OCA = core->vectaddr[VECT_TIMER1_COMPA];
	timer16_1.int_vector_OCB = core->vectaddr[VECT_TIMER1_COMPB];
	timer16_1.int_vector_OCC = core->vectaddr[VECT_TIMER1_COMPC];
	timer16_1.int_vector_OVF = core->vectaddr[VECT_TIMER1_OVF];
	timer16_1.addr_ocrbl = REG(R_OCR1BL);
	timer16_1.addr_ocrbh = REG(R_OCR1BH);
	timer16_1.addr_ocral = REG(R_OCR1AL);
	timer16_1.addr_ocrah = REG(R_OCR1AH);
	timer16_1.addr_ocrbl = REG(R_ICR1L);
	timer16_1.addr_ocrbh = REG(R_ICR1H);
	timer16_1.addr_tccra = REG(R_TCCR1A);
	timer16_1.addr_tccrb = REG(R_TCCR1B);
	timer16_1.addr_tcntl = REG(R_TCNT1L);
	timer16_1.addr_tcnth = REG(R_TCNT1H);
	timer16_1.addr_timsk = REG(R_TIMSK);
	timer16_1.addr_tifr = REG(R_TIFR);
	timer16_1.bit_mask_ocra = 4;
	timer16_1.bit_mask_ovf = 2;
	core->io_write[REG(R_OCR1BL)] = timer16_write; core->io_read[REG(R_OCR1BL)] = timer16_read; core->peripheral[REG(R_OCR1BL)] = &timer16_1;
	core->io_write[REG(R_OCR1BH)] = timer16_write; core->io_read[REG(R_OCR1BH)] = timer16_read; core->peripheral[REG(R_OCR1BH)] = &timer16_1;
	core->io_write[REG(R_OCR1AL)] = timer16_write; core->io_read[REG(R_OCR1AL)] = timer16_read; core->peripheral[REG(R_OCR1AL)] = &timer16_1;
	core->io_write[REG(R_OCR1AH)] = timer16_write; core->io_read[REG(R_OCR1AH)] = timer16_read; core->peripheral[REG(R_OCR1AH)] = &timer16_1;
	core->io_write[REG(R_ICR1L)] = timer16_write; core->io_read[REG(R_ICR1L)] = timer16_read; core->peripheral[REG(R_ICR1L)] = &timer16_1;
	core->io_write[REG(R_ICR1H)] = timer16_write; core->io_read[REG(R_ICR1H)] = timer16_read; core->peripheral[REG(R_ICR1H)] = &timer16_1;
	core->io_write[REG(R_TCCR1A)] = timer16_write; core->io_read[REG(R_TCCR1A)] = timer16_read; core->peripheral[REG(R_TCCR1A)] = &timer16_1;
	core->io_write[REG(R_TCCR1B)] = timer16_write; core->io_read[REG(R_TCCR1B)] = timer16_read; core->peripheral[REG(R_TCCR1B)] = &timer16_1;
	core->io_write[REG(R_TCNT1L)] = timer16_write; core->io_read[REG(R_TCNT1L)] = timer16_read; core->peripheral[REG(R_TCNT1L)] = &timer16_1;
	core->io_write[REG(R_TCNT1H)] = timer16_write; core->io_read[REG(R_TCNT1H)] = timer16_read; core->peripheral[REG(R_TCNT1H)] = &timer16_1;
	core->io_write[REG(R_TIMSK)] = timer16_write; core->io_read[REG(R_TIMSK)] = timer16_read; core->peripheral[REG(R_TIMSK)] = &timer16_1;
	core->io_write[REG(R_TIFR)] = timer16_write; core->io_read[REG(R_TIFR)] = timer16_read; core->peripheral[REG(R_TIFR)] = &timer16_1;
	periph_add_clock(&timer16_1, timer16_clock);
}

void list_avr_models() {
	int i = 0;

	printf("Supported MCU models:\n");
	while (1) {
		if (models[i].name == NULL) break;
		printf("%s\t%s%lu flash\t%lu SRAM\t%lu EEPROM\n",
			models[i].name, strlen(models[i].name) < 8 ? "\t" : "",
			models[i].flash_size, models[i].data_size, models[i].eeprom_size);
		i++;
	}
}

int init_avr(struct avr_core_s* core, struct avr_config_s* avr_config) {
	int i = 0, j;
	void *usart_tx[USART_MAX_COUNT];

	while (1) {
		if (models[i].name == NULL) {
			list_avr_models();
			return -1;
		}
		if (strcmp(avr_config->mcu, models[i].name) == 0) break;
		i++;
	}

	printl(LOG_INFO, "Initializing AVR model: %s\n", models[i].name);

	core->flash_size = models[i].flash_size >> 1;
	core->data_size = models[i].data_size + models[i].sram_base;
	core->eeprom_size = models[i].eeprom_size;
	core->sram_base = models[i].sram_base;
	core->xmem_size = 0;

	printl(LOG_DETAIL, " Flash: %lu bytes\n  SRAM: %lu bytes\nEEPROM: %lu bytes\n", models[i].flash_size, models[i].data_size, models[i].eeprom_size);

	core->flash = calloc(core->flash_size, 2);
	core->data = calloc(core->data_size, 1);
	core->eeprom = calloc(core->eeprom_size, 1);
	core->xmem = NULL;

	if ((core->flash == NULL) || (core->data == NULL) || (core->eeprom == NULL)) {
		printl(LOG_ERROR, "Unable to allocate for AVR memories.\n");
		return -1;
	}

	for (j = 0; j < REG_COUNT; j++) {
		core->regaddr[j] = 0;
	}

	for (j = 0; j < VECT_COUNT; j++) {
		core->vectaddr[j] = 0;
	}

	for (j = 0; j < 0x200; j++) {
		core->io_read[j] = NULL;
		core->io_write[j] = NULL;
		core->peripheral[j] = NULL;
	}

	(*models[i].device_init)(core);

	if (core->xmem_size) {
		core->xmem = calloc(core->xmem_size, 1);
		if (core->xmem == NULL) {
			printl(LOG_ERROR, "Unable to allocate for AVR memories.\n");
			return -1;
		}
	}

	core->regaddr[R_XL] = 0x1A;
	core->regaddr[R_XH] = 0x1B;
	core->regaddr[R_X] = core->regaddr[R_XL];
	core->regaddr[R_YL] = 0x1C;
	core->regaddr[R_YH] = 0x1D;
	core->regaddr[R_Y] = core->regaddr[R_YL];
	core->regaddr[R_ZL] = 0x1E;
	core->regaddr[R_ZH] = 0x1F;
	core->regaddr[R_Z] = core->regaddr[R_ZL];
	core->regaddr[R_SP] = core->regaddr[R_SPL];

	for (j = 0; j < USART_MAX_COUNT; j++) {
		switch (avr_config->usart_redirect[j]) {
		case USART_REDIRECT_NONE:
			usart_tx[j] = NULL;
			break;
		case USART_REDIRECT_STDIO:
			usart_tx[j] = (void*)usart_stdio_tx;
			break;
		case USART_REDIRECT_TCP:
			usart_tx[j] = (void*)tcpconsole_send;
			break;
		default:
			printl(LOG_ERROR, "Internal USART%d redirect value is invalid. This shouldn't be possible.\n", j);
			return -1;
		}
	}

	// ************** USARTs
	if (REG(R_UDR0)) {
		usart_init(&usart[0], usart_tx[0]);
		usart[0].int_vector_RX = core->vectaddr[VECT_USART0_RXC];
		usart[0].int_vector_UDRE = core->vectaddr[VECT_USART0_UDRE];
		usart[0].int_vector_TX = core->vectaddr[VECT_USART0_TXC];
		usart[0].addr_ucsra = REG(R_UCSR0A);
		usart[0].addr_ucsrb = REG(R_UCSR0B);
		usart[0].addr_ucsrc = REG(R_UCSR0B);
		usart[0].addr_ubrrh = REG(R_UBRR0H);
		usart[0].addr_ubrrl = REG(R_UBRR0L);
		usart[0].addr_udr = REG(R_UDR0);
		core->io_write[REG(R_UBRR0H)] = usart_write; core->io_read[REG(R_UBRR0H)] = usart_read; core->peripheral[REG(R_UBRR0H)] = &usart[0];
		core->io_write[REG(R_UBRR0L)] = usart_write; core->io_read[REG(R_UBRR0L)] = usart_read; core->peripheral[REG(R_UBRR0L)] = &usart[0];
		core->io_write[REG(R_UCSR0C)] = usart_write; core->io_read[REG(R_UCSR0C)] = usart_read; core->peripheral[REG(R_UCSR0C)] = &usart[0];
		core->io_write[REG(R_UCSR0B)] = usart_write; core->io_read[REG(R_UCSR0B)] = usart_read; core->peripheral[REG(R_UCSR0B)] = &usart[0];
		core->io_write[REG(R_UCSR0A)] = usart_write; core->io_read[REG(R_UCSR0A)] = usart_read; core->peripheral[REG(R_UCSR0A)] = &usart[0];
		core->io_write[REG(R_UDR0)] = usart_write; core->io_read[REG(R_UDR0)] = usart_read; core->peripheral[REG(R_UDR0)] = &usart[0];
		periph_add_clock(&usart[0], usart_clock);
		usart[0].usart_index = 0;
	}
	else if (REG(R_UDR)) { //it's just USART instead of USART0 on parts with only one USART
		usart_init(&usart[0], usart_tx[0]);
		usart[0].int_vector_RX = core->vectaddr[VECT_USART_RXC];
		usart[0].int_vector_UDRE = core->vectaddr[VECT_USART_UDRE];
		usart[0].int_vector_TX = core->vectaddr[VECT_USART_TXC];
		usart[0].addr_ucsra = REG(R_UCSRA);
		usart[0].addr_ucsrb = REG(R_UCSRB);
		usart[0].addr_ucsrc = REG(R_UCSRB);
		usart[0].addr_ubrrh = REG(R_UBRRH);
		usart[0].addr_ubrrl = REG(R_UBRRL);
		usart[0].addr_udr = REG(R_UDR);
		core->io_write[REG(R_UBRRH)] = usart_write; core->io_read[REG(R_UBRRH)] = usart_read; core->peripheral[REG(R_UBRRH)] = &usart[0];
		core->io_write[REG(R_UBRRL)] = usart_write; core->io_read[REG(R_UBRRL)] = usart_read; core->peripheral[REG(R_UBRRL)] = &usart[0];
		core->io_write[REG(R_UCSRC)] = usart_write; core->io_read[REG(R_UCSRC)] = usart_read; core->peripheral[REG(R_UCSRC)] = &usart[0];
		core->io_write[REG(R_UCSRB)] = usart_write; core->io_read[REG(R_UCSRB)] = usart_read; core->peripheral[REG(R_UCSRB)] = &usart[0];
		core->io_write[REG(R_UCSRA)] = usart_write; core->io_read[REG(R_UCSRA)] = usart_read; core->peripheral[REG(R_UCSRA)] = &usart[0];
		core->io_write[REG(R_UDR)] = usart_write; core->io_read[REG(R_UDR)] = usart_read; core->peripheral[REG(R_UDR)] = &usart[0];
		periph_add_clock(&usart[0], usart_clock);
		usart[0].usart_index = 0;
	}

	if (REG(R_UDR1)) {
		usart_init(&usart[1], usart_tx[1]);
		usart[1].int_vector_RX = core->vectaddr[VECT_USART1_RXC];
		usart[1].int_vector_UDRE = core->vectaddr[VECT_USART1_UDRE];
		usart[1].int_vector_TX = core->vectaddr[VECT_USART1_TXC];
		usart[1].addr_ucsra = REG(R_UCSR1A);
		usart[1].addr_ucsrb = REG(R_UCSR1B);
		usart[1].addr_ucsrc = REG(R_UCSR1B);
		usart[1].addr_ubrrh = REG(R_UBRR1H);
		usart[1].addr_ubrrl = REG(R_UBRR1L);
		usart[1].addr_udr = REG(R_UDR1);
		core->io_write[REG(R_UBRR1H)] = usart_write; core->io_read[REG(R_UBRR1H)] = usart_read; core->peripheral[REG(R_UBRR1H)] = &usart[1];
		core->io_write[REG(R_UBRR1L)] = usart_write; core->io_read[REG(R_UBRR1L)] = usart_read; core->peripheral[REG(R_UBRR1L)] = &usart[1];
		core->io_write[REG(R_UCSR1C)] = usart_write; core->io_read[REG(R_UCSR1C)] = usart_read; core->peripheral[REG(R_UCSR1C)] = &usart[1];
		core->io_write[REG(R_UCSR1B)] = usart_write; core->io_read[REG(R_UCSR1B)] = usart_read; core->peripheral[REG(R_UCSR1B)] = &usart[1];
		core->io_write[REG(R_UCSR1A)] = usart_write; core->io_read[REG(R_UCSR1A)] = usart_read; core->peripheral[REG(R_UCSR1A)] = &usart[1];
		core->io_write[REG(R_UDR1)] = usart_write; core->io_read[REG(R_UDR1)] = usart_read; core->peripheral[REG(R_UDR1)] = &usart[1];
		periph_add_clock(&usart[1], usart_clock);
		usart[1].usart_index = 1;
	}

	if (REG(R_UDR2)) {
		usart_init(&usart[2], usart_tx[2]);
		usart[2].int_vector_RX = core->vectaddr[VECT_USART2_RXC];
		usart[2].int_vector_UDRE = core->vectaddr[VECT_USART2_UDRE];
		usart[2].int_vector_TX = core->vectaddr[VECT_USART2_TXC];
		usart[2].addr_ucsra = REG(R_UCSR2A);
		usart[2].addr_ucsrb = REG(R_UCSR2B);
		usart[2].addr_ucsrc = REG(R_UCSR2B);
		usart[2].addr_ubrrh = REG(R_UBRR2H);
		usart[2].addr_ubrrl = REG(R_UBRR2L);
		usart[2].addr_udr = REG(R_UDR2);
		core->io_write[REG(R_UBRR2H)] = usart_write; core->io_read[REG(R_UBRR2H)] = usart_read; core->peripheral[REG(R_UBRR2H)] = &usart[2];
		core->io_write[REG(R_UBRR2L)] = usart_write; core->io_read[REG(R_UBRR2L)] = usart_read; core->peripheral[REG(R_UBRR2L)] = &usart[2];
		core->io_write[REG(R_UCSR2C)] = usart_write; core->io_read[REG(R_UCSR2C)] = usart_read; core->peripheral[REG(R_UCSR2C)] = &usart[2];
		core->io_write[REG(R_UCSR2B)] = usart_write; core->io_read[REG(R_UCSR2B)] = usart_read; core->peripheral[REG(R_UCSR2B)] = &usart[2];
		core->io_write[REG(R_UCSR2A)] = usart_write; core->io_read[REG(R_UCSR2A)] = usart_read; core->peripheral[REG(R_UCSR2A)] = &usart[2];
		core->io_write[REG(R_UDR2)] = usart_write; core->io_read[REG(R_UDR2)] = usart_read; core->peripheral[REG(R_UDR2)] = &usart[2];
		periph_add_clock(&usart[2], usart_clock);
		usart[2].usart_index = 2;
	}

	if (REG(R_UDR3)) {
		usart_init(&usart[3], usart_tx[3]);
		usart[3].int_vector_RX = core->vectaddr[VECT_USART3_RXC];
		usart[3].int_vector_UDRE = core->vectaddr[VECT_USART3_UDRE];
		usart[3].int_vector_TX = core->vectaddr[VECT_USART3_TXC];
		usart[3].addr_ucsra = REG(R_UCSR3A);
		usart[3].addr_ucsrb = REG(R_UCSR3B);
		usart[3].addr_ucsrc = REG(R_UCSR3B);
		usart[3].addr_ubrrh = REG(R_UBRR3H);
		usart[3].addr_ubrrl = REG(R_UBRR3L);
		usart[3].addr_udr = REG(R_UDR3);
		core->io_write[REG(R_UBRR3H)] = usart_write; core->io_read[REG(R_UBRR3H)] = usart_read; core->peripheral[REG(R_UBRR3H)] = &usart[3];
		core->io_write[REG(R_UBRR3L)] = usart_write; core->io_read[REG(R_UBRR3L)] = usart_read; core->peripheral[REG(R_UBRR3L)] = &usart[3];
		core->io_write[REG(R_UCSR3C)] = usart_write; core->io_read[REG(R_UCSR3C)] = usart_read; core->peripheral[REG(R_UCSR3C)] = &usart[3];
		core->io_write[REG(R_UCSR3B)] = usart_write; core->io_read[REG(R_UCSR3B)] = usart_read; core->peripheral[REG(R_UCSR3B)] = &usart[3];
		core->io_write[REG(R_UCSR3A)] = usart_write; core->io_read[REG(R_UCSR3A)] = usart_read; core->peripheral[REG(R_UCSR3A)] = &usart[3];
		core->io_write[REG(R_UDR3)] = usart_write; core->io_read[REG(R_UDR3)] = usart_read; core->peripheral[REG(R_UDR3)] = &usart[3];
		periph_add_clock(&usart[3], usart_clock);
		usart[3].usart_index = 3;
	}


	// ************** ADC
	if (REG(R_ADCSRA)) {
		adc_init(&adc);
		adc.int_vector_adc = core->vectaddr[VECT_ADC];
		adc.addr_adch = REG(R_ADCH);
		adc.addr_adcl = REG(R_ADCL);
		adc.addr_adcsra = REG(R_ADCSRA);
		adc.addr_admux = REG(R_ADMUX);
		core->io_write[REG(R_ADCH)] = adc_write; core->io_read[REG(R_ADCH)] = adc_read; core->peripheral[REG(R_ADCH)] = &adc;
		core->io_write[REG(R_ADCL)] = adc_write; core->io_read[REG(R_ADCL)] = adc_read; core->peripheral[REG(R_ADCL)] = &adc;
		core->io_write[REG(R_ADCSRA)] = adc_write; core->io_read[REG(R_ADCSRA)] = adc_read; core->peripheral[REG(R_ADCSRA)] = &adc;
		core->io_write[REG(R_ADMUX)] = adc_write; core->io_read[REG(R_ADMUX)] = adc_read; core->peripheral[REG(R_ADMUX)] = &adc;
		periph_add_clock(&adc, adc_clock);
	}

	return 0;
}
