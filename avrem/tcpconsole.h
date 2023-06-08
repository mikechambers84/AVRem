#ifndef _TCPCONSOLE_H_
#define _TCPCONSOLE_H_

#include <stdint.h>
#include "usart.h"

int tcpconsole_init(struct usart_s* usart, uint16_t port);
void tcpconsole_dorecv(struct usart_s* usart);
void tcpconsole_send(struct usart_s* usart, uint8_t val);
void tcpconsole_send_array(struct usart_s* usart, uint8_t* data, int len);

#endif
