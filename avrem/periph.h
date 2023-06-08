#ifndef _PERIPH_H_
#define _PERIPH_H_

#include <stdint.h>
#include "usart.h"
#include "timer16.h"
#include "avrcore.h"

void periph_clock(struct avr_core_s* core, int cycles);
void periph_add_clock(void* peripheral, void (*clock_func)(struct avr_core_s* core, void* add_peripheral, int cycles));

#endif
