#include <stdint.h>
#include <stdlib.h>
#include "periph.h"
#include "avrcore.h"

void (*clock_callbacks[0x200])(struct avr_core_s* core, void* peripheral_data, int cycles);
void* peripheral_data[0x200];
int peripheral_count = 0;

void periph_clock(struct avr_core_s* core, int cycles) {
	int i;
	for (i = 0; i < peripheral_count; i++) {
		if (clock_callbacks[i] != NULL) {
			(*clock_callbacks[i])(core, peripheral_data[i], cycles);
		}
	}
}

void periph_add_clock(void* peripheral, void (*clock_func)(struct avr_core_s* core, void* add_peripheral, int cycles)) {
	if (peripheral_count == 0x200) return;

	clock_callbacks[peripheral_count] = clock_func;
	peripheral_data[peripheral_count] = peripheral;
	peripheral_count++;
}
