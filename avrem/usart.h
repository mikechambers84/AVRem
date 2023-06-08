#ifndef _USART_H_
#define _USART_H_

#include <stdint.h>
#include "avrcore.h"

#define USART_TX_BUFFER_SIZE 16 //TODO: How large is the actual transmit buffer supposed to be?
#define USART_RX_BUFFER_SIZE 16 //TODO: How large is the actual receive buffer supposed to be?

struct usart_s {
	uint8_t ucsra;
	uint8_t ucsrb;
	uint8_t ucsrc;

	uint8_t ubrrh;
	uint8_t ubrrl;

	uint32_t counter;
	uint32_t ubrr;
	uint8_t div;
	uint8_t divcounter;

	uint8_t rxdata[USART_RX_BUFFER_SIZE];
	uint8_t txdata[USART_TX_BUFFER_SIZE];
	uint8_t rxidx;
	uint8_t txidx;

	uint32_t int_vector_TX;
	uint32_t int_vector_RX;
	uint32_t int_vector_UDRE;

	uint32_t addr_ucsra;
	uint32_t addr_ucsrb;
	uint32_t addr_ucsrc;
	uint32_t addr_ubrrh;
	uint32_t addr_ubrrl;
	uint32_t addr_udr;

	uint8_t usart_index;

	void (*callback_tx)(struct usart_s* usart, uint8_t val);
};

void usart_stdio_tx(struct usart_s* usart, uint8_t val);
void usart_rx(struct usart_s* usart, uint8_t val);
void usart_write(struct avr_core_s* core, struct usart_s* usart, uint32_t addr, uint8_t val);
uint8_t usart_read(struct avr_core_s* core, struct usart_s* usart, uint32_t addr);
void usart_clock(struct avr_core_s* core, struct usart_s* usart, int clocks);
void usart_init(struct usart_s* usart, void (*callback_tx)(struct usart_s* usart, uint8_t val));

#endif
