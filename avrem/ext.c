#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "avrcore.h"
#include "adc.h"
#include "tcpconsole.h"

/*

	External interface protocol

	All multi-byte values are transmitted in little-endian.

	Incoming command format:

	Offset | Description
	-------|-----------------------------------------------
	   0   | Command ID
	 1...6 | Data
	   7   | Inverted checksum

	Incoming commands:

	Cmd ID | Description
	-------|-----------------------------------------------
	   1   | Register IO write callback (data 1-2 = address)
	   2   | Register IO read callback (data 1-2 = address)
	   3   | Set IO memory (data 1-2 = address, data 3 = value)
	   4   | Reserved
	   5   | Reserved
	   6   | Reserved
	   7   | Reserved
	   8   | Reserved
	   9   | Change ADC input value (data 1 = ADC channel, data 2-3 = value)
	  10   | Start emulation if it was waiting on startup

*/

extern struct avr_core_s avr;
extern struct adc_s adc;

uint8_t extbuf[8];
volatile int ext_start = 0;

uint8_t ext_calc_checksum(uint8_t* data) {
	uint8_t cksum = 0, i;
	for (i = 0; i < 7; i++) {
		cksum += data[i];
	}
	cksum ^= 0xFF;
	return cksum;
}

void ext_write_callback(struct avr_core_s* core, void* peripheral, uint32_t addr, uint8_t val) {
	uint8_t outarray[8];
	outarray[0] = 1; //IO out command
	outarray[1] = (uint8_t)addr;
	outarray[2] = (uint8_t)(addr >> 8);
	outarray[3] = val;
	outarray[4] = 0;
	outarray[5] = 0;
	outarray[6] = 0;
	outarray[7] = ext_calc_checksum(outarray);
	tcpconsole_send_array(NULL, outarray, 8);
}

void ext_rx(uint8_t val) {
	int i;
	uint8_t cksum;
	uint32_t addr;

	for (i = 1; i < 8; i++) {
		extbuf[i - 1] = extbuf[i];
	}
	extbuf[7] = val;

	if (ext_calc_checksum(extbuf) != extbuf[7]) return;

	switch (extbuf[0]) {
	/*case 1: //buttons update
		avr.data[avr.regaddr[R_PIND]] = extbuf[1] ^ 0xF8;
		break;*/
	case 1: //register IO write callback
		addr = (uint32_t)extbuf[1] | ((uint16_t)extbuf[2] << 8);
		avr.io_write[addr] = ext_write_callback;
		break;
	case 2: //register IO read callback
		//TODO
		break;
	case 3: //set IO memory
		addr = (uint32_t)extbuf[1] | ((uint16_t)extbuf[2] << 8);
		avr.data[addr] = extbuf[3];
		break;
	case 9: //ADC change
		adc_setinput(&adc, extbuf[1], (uint16_t)extbuf[2] | ((uint16_t)extbuf[3] << 8));
		break;
	case 10: //tell AVRem to start in case it's waiting for the external interface
		ext_start = 1;
		break;
	}
}

