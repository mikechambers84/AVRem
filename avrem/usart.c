#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "avrcore.h"
#include "usart.h"

void usart_stdio_tx(struct usart_s* usart, uint8_t val) {
	printf("%c", val);
	fflush(stdout);
}

void usart_rx(struct usart_s* usart, uint8_t val) {
	if (usart == NULL) return;
	if ((usart->ucsrb & 0x10) == 0) return; //make sure RXEN is set
	if (usart->rxidx == USART_RX_BUFFER_SIZE) return; //buffer's already full
	usart->rxdata[usart->rxidx++] = val;
	usart->ucsra |= (1 << 7);
}

void usart_tx(struct usart_s* usart, uint8_t val) {
	if (usart == NULL) return;
	if (usart->txidx == USART_TX_BUFFER_SIZE) return; //buffer's already full
	usart->txdata[usart->txidx++] = val;
	usart->ucsra &= ~(1 << 6);
	usart->ucsra &= ~(1 << 5);
}

void usart_write(struct avr_core_s* core, struct usart_s* usart, uint32_t addr, uint8_t val) {
	if (usart == NULL) return;
	if (addr == usart->addr_ucsra) { //UCSRnA
		if (val & (1 << 6)) val &= ~(1 << 6); //writing 1 to TXC will clear it
		if (val & 0x02) usart->div = 8; else usart->div = 16; //U2X set to 1 will double the transmission rate
		usart->ucsra = (usart->ucsra & 0xBC) | (val & 0x43);
	}
	else if (addr == usart->addr_ucsrb) { //UCSRnB
		if ((val & (1 << 4)) == 0) usart->ucsra &= ~(1 << 7); //clear RXC in UCSRA if receiver disabled
		if (((usart->ucsrb & (1 << 3)) == 0) && (val & (1 << 3))) usart->ucsra |= 1 << 5; //set UDRE when toggling from TXEN off to on
		usart->ucsrb = val;
	}
	else if (addr == usart->addr_ucsrc) { //UCSRnC
		usart->ucsrc = val;
	}
	else if (addr == usart->addr_ubrrl) { //UBRRnL
		usart->ubrrl = val;
		usart->ubrr = (uint16_t)val | (((uint16_t)usart->ubrrh & 0x0F) << 8);
	}
	else if (addr == usart->addr_ubrrh) { //UBRRnH
		usart->ubrrh = val;
	}
	else if (addr == usart->addr_udr) { //UDRn
		if (usart->ucsrb & 0x08) { //make sure TXEN is set
			usart_tx(usart, val);
		}
	}
}

uint8_t usart_read(struct avr_core_s* core, struct usart_s* usart, uint32_t addr) {
	uint8_t ret = 0;
	if (usart == NULL) return 0;
	if (addr == usart->addr_ucsra) { //UCSRnA
		ret = usart->ucsra;
	}
	else if (addr == usart->addr_ucsrb) { //UCSRnB
		ret = usart->ucsrb;
	}
	else if (addr == usart->addr_ucsrc) { //UCSRnC
		ret = usart->ucsrc;
	}
	else if (addr == usart->addr_ubrrl) { //UBRRnL
		ret = usart->ubrrl;
	}
	else if (addr == usart->addr_ubrrh) { //UBRRnH
		ret = usart->ubrrh;
	}
	else if (addr == usart->addr_udr) { //UDRn
		int i;
		ret = usart->rxdata[0];
		for (i = 1; i < usart->rxidx; i++) {
			usart->rxdata[i - 1] = usart->rxdata[i];
		}
		if (usart->rxidx > 0) {
			usart->rxidx--;
		}
		if (usart->rxidx == 0) {
			usart->ucsra &= ~(1 << 7); //clear RXC if buffer is empty
		}
	}
	return ret;
}

void usart_clock(struct avr_core_s* core, struct usart_s* usart, int clocks) {
	int i;
	if (usart == NULL) return;

usart_clock_again:
	if (usart->counter == 0) {
		usart->counter = usart->ubrr;
		if (usart->counter == 0) usart->counter = 1;
		if (++usart->divcounter >= usart->div) {
			usart->divcounter = 0;
			if (usart->txidx > 0) {
				if (usart->callback_tx != NULL) {
					(*usart->callback_tx)(usart, usart->txdata[0]);
				}
				for (i = 1; i < USART_TX_BUFFER_SIZE; i++) {
					usart->txdata[i - 1] = usart->txdata[i];
				}
				usart->txidx--;
				usart->ucsra |= (1 << 5); //UDRE on every byte transferred out of buffer
				if (usart->txidx == 0) {
					usart->ucsra |= (1 << 6); //TXC only on entire transmit buffer being empty
				}
			}
		}
	}

	usart->counter--;

	if (--clocks > 0) goto usart_clock_again;

	//interrupts
	if (usart->ucsra & usart->ucsrb & (1 << 7)) { //RXC
		avr_interrupt(core, usart->int_vector_RX);
	}
	else if (usart->ucsra & usart->ucsrb & (1 << 6)) { //TXC
		if (avr_interrupt(core, usart->int_vector_TX)) { //If interrupt was exectued, clear TXC
			usart->ucsra &= ~(1 << 6);
		}
	}
	else if (usart->ucsra & usart->ucsrb & (1 << 5)) { //UDRE
		avr_interrupt(core, usart->int_vector_UDRE);
	}
}

void usart_init(struct usart_s* usart, void (*callback_tx)(struct usart_s* usart, uint8_t val)) {
	if (usart == NULL) return;
	usart_write(NULL, usart, 0, 0);
	usart_write(NULL, usart, 1, 0);
	usart_write(NULL, usart, 2, 0);
	usart_write(NULL, usart, 4, 0);
	usart_write(NULL, usart, 5, 0);
	usart->ucsra = (1 << 5);
	usart->ucsrc = (1 << 2) | (1 << 1);
	usart->counter = 1;
	usart->rxidx = 0;
	usart->txidx = 0;
	usart->div = 16;
	usart->divcounter = 0;
	usart->callback_tx = callback_tx;
}
