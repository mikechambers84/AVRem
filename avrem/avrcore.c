#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <conio.h>
#include "config.h"
#include "eeprom.h"
#include "usart.h"
#include "timer16.h"
#include "periph.h"
#include "tcpconsole.h"
#include "module.h"
#include "avrcore.h"

int trace = 0;

inline void avr_data_write(struct avr_core_s* core, uint32_t addr, uint8_t val) {
	if (addr >= core->data_size) {
		if (addr < core->xmem_size) {
			if (core->data[REG(R_XMCRA)] & (1 << 7)) {
				core->xmem[addr] = val; //only if SRE set
				//printf("write XMEM %04X %02X\n", addr, val);
			}
			else return;
		}
		else return;	
	}

	module_do_io_write(addr, &val);

	//if (trace)
	//	printf("Data write %02X to %04X\n", val, addr);

	//IO range
	if ((addr >= 0x20) && (addr < core->sram_base)) {
		uint8_t prevval;
		prevval = core->data[addr];

		//printf("IO write to %03X\n", addr);
		/*if (addr == 0x23) {
			printf("Debug: %u\n", val);
		}*/

		if (core->io_write[addr] != NULL) {
			(*core->io_write[addr])(core, core->peripheral[addr], addr, val);
		}

		else {
			core->data[addr] = val;
		}

	}

	else {
		core->data[addr] = val;
	}
}

inline void avr_data_write_w(struct avr_core_s* core, uint32_t addr, uint16_t val) {
	avr_data_write(core, addr, (uint8_t)val);
	val >>= 8;
	avr_data_write(core, addr + 1, (uint8_t)val);
}

inline uint8_t avr_data_read(struct avr_core_s* core, uint32_t addr) {
	uint8_t ret = 0xFF;
	if (addr >= core->data_size) {
		if (addr < core->xmem_size) {
			if (core->data[REG(R_XMCRA)] & (1 << 7)) {
				ret = core->xmem[addr]; //only if SRE set
				//printf("read XMEM %04X %02X\n", addr, ret);
			}
		}
	}
	else ret = core->data[addr];

	//IO range

	if ((addr >= 0x20) && (addr < core->sram_base)) {

		if (core->io_read[addr] != NULL) {
			ret = (*core->io_read[addr])(core, core->peripheral[addr], addr);
		}

	}

	module_do_io_read(addr, &ret);

	return ret;
}

inline uint16_t avr_data_read_w(struct avr_core_s* core, uint32_t addr) {
	uint16_t ret;
	ret = (uint16_t)avr_data_read(core, addr + 1);
	ret <<= 8;
	ret |= (uint16_t)avr_data_read(core, addr);
	return ret;
}

void avr_flash_write_byte(struct avr_core_s* core, uint32_t addr, uint8_t val) {
	uint32_t ea;
	ea = addr >> 1;
	//ea &= core->addr_mask;
	if (ea >= core->flash_size) {
		printf("Write to %04X: %02X FAILED!\n", addr, val);
		return;
	}
	if (addr & 1) { //high byte
		core->flash[ea] = ((uint16_t)val << 8) | (core->flash[ea] & 0x00FF);
	}
	else { //low byte
		core->flash[ea] = (uint16_t)val | (core->flash[ea] & 0xFF00);
	}
}

uint8_t avr_flash_read_byte(struct avr_core_s* core, uint32_t addr) {
	uint32_t ea;
	ea = addr >> 1;
	//ea &= core->addr_mask;
	if (ea >= core->flash_size) {
		printf("Read from %04X FAILED!\n", addr);
		return 0xFF;
	}
	if (addr & 1) { //high byte
		return (uint8_t)(core->flash[ea] >> 8);
	}
	else { //low byte
		return (uint8_t)core->flash[ea];
	}
}

void avr_flash_write(struct avr_core_s* core, uint32_t addr, uint16_t val) {
	//printf("Write to %04X: %02X\n", addr, val);
	//addr &= core->addr_mask;
	if (addr >= core->flash_size) return;
	core->flash[addr] = val;
}

uint16_t avr_flash_read(struct avr_core_s* core, uint32_t addr) {
	//addr &= core->addr_mask;
	if (addr >= core->flash_size) return 0xFF;
	return core->flash[addr];
}

void avr_io_out(struct avr_core_s* core, uint32_t port, uint8_t val) {
	//if (trace)
	//	printf("Port out %08X = %02X (PC %08X)\n", port, val, core->pc << 1);
	port += 0x20;
	avr_data_write(core, port, val);
}

uint8_t avr_io_in(struct avr_core_s* core, uint32_t port) {
	port += 0x20;
	return avr_data_read(core, port);
}

void avr_push_byte(struct avr_core_s* core, uint8_t val) {
	uint16_t sp;
	sp = avr_data_read_w(core, REG(R_SP));
	if (trace)
		printf("push byte SP = %04X: %02X\n", sp, val);
	avr_data_write(core, sp, val);
	sp--;
	avr_data_write_w(core, REG(R_SP), sp);
}

uint8_t avr_pop_byte(struct avr_core_s* core) {
	uint16_t sp;
	uint8_t val;
	sp = avr_data_read_w(core, REG(R_SP)) + 1;
	val = avr_data_read(core, sp);
	avr_data_write_w(core, REG(R_SP), sp);
	if (trace)
		printf(" pop byte SP = %04X: %02X\n", sp, val);
	return val;
}

void avr_push_addr(struct avr_core_s* core, uint32_t addr) {
	uint16_t sp, i;
	sp = avr_data_read_w(core, REG(R_SP));
	for (i = 0; i < core->addr_size; i++) {
		avr_data_write(core, sp, (uint8_t)addr);
		if (trace)
			printf("push addr SP = %04X: %02X\n", sp, (uint8_t)addr);
		addr >>= 8;
		sp--;
	}
	avr_data_write_w(core, REG(R_SP), sp);
}

uint32_t avr_pop_addr(struct avr_core_s* core) {
	uint16_t sp, i;
	uint32_t addr = 0;
	sp = avr_data_read_w(core, REG(R_SP));
	for (i = 0; i < core->addr_size; i++) {
		sp++;
		addr <<= 8;
		addr |= (uint32_t)avr_data_read(core, sp);
		if (trace)
			printf(" pop addr SP = %04X: %02X\n", sp, (uint8_t)addr);
	}
	avr_data_write_w(core, REG(R_SP), sp);
	return addr;
}

inline void avr_bitwise_flags(struct avr_core_s* core, uint8_t result) {
	if (result & 0x80) {
		REG_SET_BIT(core, R_SREG, SREG_N);
		REG_SET_BIT(core, R_SREG, SREG_S);
	}
	else {
		REG_CLR_BIT(core, R_SREG, SREG_N);
		REG_CLR_BIT(core, R_SREG, SREG_S);
	}

	if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
	else REG_CLR_BIT(core, R_SREG, SREG_Z);

	REG_CLR_BIT(core, R_SREG, SREG_V);
}

inline uint16_t avr_add_w(struct avr_core_s* core, uint16_t dst, uint16_t src) {
	uint16_t result;

	result = dst + src;

	if ((~dst & result) & 0x8000) REG_SET_BIT(core, R_SREG, SREG_V);
	else REG_CLR_BIT(core, R_SREG, SREG_V);

	if ((dst & ~result) & 0x8000) REG_SET_BIT(core, R_SREG, SREG_C);
	else REG_CLR_BIT(core, R_SREG, SREG_C);

	if (result & 0x8000) REG_SET_BIT(core, R_SREG, SREG_N);
	else REG_CLR_BIT(core, R_SREG, SREG_N);

	if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
	else REG_CLR_BIT(core, R_SREG, SREG_S);

	if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
	else REG_CLR_BIT(core, R_SREG, SREG_Z);

	return result;
}

inline uint8_t avr_add(struct avr_core_s* core, uint8_t dst, uint8_t src) {
	uint8_t result, carry;

	result = dst + src;
	carry = (dst & src) | (src & ~result) | (dst & ~result);

	if (carry & 0x08) REG_SET_BIT(core, R_SREG, SREG_H);
	else REG_CLR_BIT(core, R_SREG, SREG_H);

	if (carry & 0x80) REG_SET_BIT(core, R_SREG, SREG_C);
	else REG_CLR_BIT(core, R_SREG, SREG_C);

	if (((dst & src & ~result) | (~dst & ~src & result)) & 0x80) REG_SET_BIT(core, R_SREG, SREG_V);
	else REG_CLR_BIT(core, R_SREG, SREG_V);

	if (result & 0x80) REG_SET_BIT(core, R_SREG, SREG_N);
	else REG_CLR_BIT(core, R_SREG, SREG_N);

	if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
	else REG_CLR_BIT(core, R_SREG, SREG_S);

	if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
	else REG_CLR_BIT(core, R_SREG, SREG_Z);

	return result;
}

inline uint8_t avr_add_c(struct avr_core_s* core, uint8_t dst, uint8_t src, uint8_t c) {
	uint8_t result, carry;

	result = dst + src + c;
	carry = (dst & src) | (src & ~result) | (dst & ~result);

	if (carry & 0x08) REG_SET_BIT(core, R_SREG, SREG_H);
	else REG_CLR_BIT(core, R_SREG, SREG_H);

	if (carry & 0x80) REG_SET_BIT(core, R_SREG, SREG_C);
	else REG_CLR_BIT(core, R_SREG, SREG_C);

	if (((dst & src & ~result) | (~dst & ~src & result)) & 0x80) REG_SET_BIT(core, R_SREG, SREG_V);
	else REG_CLR_BIT(core, R_SREG, SREG_V);

	if (result & 0x80) 	REG_SET_BIT(core, R_SREG, SREG_N);
	else REG_CLR_BIT(core, R_SREG, SREG_N);

	if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
	else REG_CLR_BIT(core, R_SREG, SREG_S);

	if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
	else REG_CLR_BIT(core, R_SREG, SREG_Z);

	return result;
}

inline uint16_t avr_sub_w(struct avr_core_s* core, uint16_t dst, uint16_t src) {
	uint16_t result;

	result = dst - src;

	if ((dst & ~result) & 0x8000) REG_SET_BIT(core, R_SREG, SREG_V);
	else REG_CLR_BIT(core, R_SREG, SREG_V);

	if ((~dst & result) & 0x8000) REG_SET_BIT(core, R_SREG, SREG_C);
	else REG_CLR_BIT(core, R_SREG, SREG_C);

	if (result & 0x8000) REG_SET_BIT(core, R_SREG, SREG_N);
	else REG_CLR_BIT(core, R_SREG, SREG_N);

	if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
	else REG_CLR_BIT(core, R_SREG, SREG_S);

	if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
	else REG_CLR_BIT(core, R_SREG, SREG_Z);

	return result;
}

inline uint8_t avr_sub(struct avr_core_s* core, uint8_t dst, uint8_t src) {
	uint8_t result, carry;

	result = dst - src;
	carry = (~dst & src) | (src & result) | (~dst & result);

	if (carry & 0x08) REG_SET_BIT(core, R_SREG, SREG_H);
	else REG_CLR_BIT(core, R_SREG, SREG_H);

	if (carry & 0x80) REG_SET_BIT(core, R_SREG, SREG_C);
	else REG_CLR_BIT(core, R_SREG, SREG_C);

	if (((dst & ~src & ~result) | (~dst & src & result)) & 0x80) REG_SET_BIT(core, R_SREG, SREG_V);
	else REG_CLR_BIT(core, R_SREG, SREG_V);

	if (result & 0x80) 	REG_SET_BIT(core, R_SREG, SREG_N);
	else REG_CLR_BIT(core, R_SREG, SREG_N);

	if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
	else REG_CLR_BIT(core, R_SREG, SREG_S);

	if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
	else REG_CLR_BIT(core, R_SREG, SREG_Z);

	return result;
}

inline uint8_t avr_sub_c(struct avr_core_s* core, uint8_t dst, uint8_t src, uint8_t c) {
	uint8_t result, carry;

	result = dst - src - c;
	carry = (~dst & src) | (src & result) | (~dst & result);

	if (carry & 0x08) REG_SET_BIT(core, R_SREG, SREG_H);
	else REG_CLR_BIT(core, R_SREG, SREG_H);

	if (carry & 0x80) REG_SET_BIT(core, R_SREG, SREG_C);
	else REG_CLR_BIT(core, R_SREG, SREG_C);

	if (((dst & ~src & ~result) | (~dst & src & result)) & 0x80) REG_SET_BIT(core, R_SREG, SREG_V);
	else REG_CLR_BIT(core, R_SREG, SREG_V);

	if (result & 0x80) 	REG_SET_BIT(core, R_SREG, SREG_N);
	else REG_CLR_BIT(core, R_SREG, SREG_N);

	if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
	else REG_CLR_BIT(core, R_SREG, SREG_S);

	if (result) REG_CLR_BIT(core, R_SREG, SREG_Z);

	return result;
}

void avr_reset(struct avr_core_s* core) {
	core->sleeping = 0;
	core->pc = 0;
	core->addr_mask = core->flash_size - 1;
	if (core->flash_size < 256)
		core->addr_size = 1;
	else if (core->flash_size < 65536)
		core->addr_size = 2;
	else
		core->addr_size = 3;
	avr_data_write(core, REG(R_SREG), 0);
	avr_data_write_w(core, REG(R_SP), (uint16_t)(core->data_size - 1));
}

void avr_dbg_check(struct avr_core_s* core, uint16_t op);
FILE* dbg = NULL;

int avr_execute(struct avr_core_s* core, int cycles) {
	uint16_t opcode;
	int invalid, opcycles;
	uint8_t result, K, bit, tempval;
	uint16_t val16;
	uint32_t k, ea, branchdst, a5, a6, r4, r5, d4, d5, q6;

	//if (dbg == NULL) {
	//	dbg = fopen("c:\\git\\trace.bin", "rb");
	//}

execute:
	invalid = 0;
	opcycles = 1;
	if (core->sleeping) goto skip_set_pc;
	//core->pc &= core->addr_mask;
	if (core->pc >= core->flash_size) {
		printf("CRASH: PC = %08X\n", core->pc);
		return 0x80;
	}
	opcode = core->flash[core->pc];
	if (core->skip_next) {
		core->skip_next = 0;
		if (((opcode & 0xFE0F) == 0x9200) || ((opcode & 0xFE0F) == 0x9000)
			|| ((opcode & 0xFE0E) == 0x940C) || ((opcode & 0xFE0E) == 0x940D)
			|| ((opcode & 0xFE0E) == 0x940E) || ((opcode & 0xFE0E) == 0x940F)) { //two-byte instructions
			core->pc++; //so skip another byte
			opcycles++; //and clock another cycle
		}
		goto skip_instruction;
	}

	if (trace)
		printf("%08X %08X: %04X ", core->pc, core->pc << 1, opcode);
	//printf("%08X (%08X) SP = %08X\n", core->pc, core->pc << 1, avr_data_read_w(core, R_SP));

	switch (opcode >> 12) {
	case 0x0:
		if (opcode == 0x0000) { //NOP 0000 0000 0000 0000
			//nothing
		}
		else if ((opcode & 0xFF00) == 0x0100) { //MOVW 0000 0001 dddd rrrr
			r4 = (opcode & 0xF) << 1;
			d4 = ((opcode >> 4) & 0xF) << 1;
			val16 = avr_data_read_w(core, r4);
			avr_data_write_w(core, d4, val16);
			if (trace)
				printf("MOVW r%d, r%d (%04X)", d4, r4, val16);
		}
		else if ((opcode & 0xFF00) == 0x0200) { //MULS 0000 0010 dddd rrrr
			int16_t muls;
			r5 = (opcode & 0xF) + 16;
			d5 = ((opcode >> 4) & 0xF) + 16;
			muls = (int8_t)avr_data_read(core, d5) * (int8_t)avr_data_read(core, r5);
			avr_data_write_w(core, 0, (uint16_t)muls);

			if (muls & 0x8000) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);

			if (muls == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			opcycles++;

			if (trace)
				printf("MULS r%d, r%d", d5, r5);
		}
		else if ((opcode & 0xFF88) == 0x0300) { //MULSU 0000 0011 0ddd 0rrr
			int16_t mul;
			r5 = (opcode & 0x7) + 16;
			d5 = ((opcode >> 4) & 0x7) + 16;
			mul = ((int8_t)avr_data_read(core, d5)) * ((uint8_t)avr_data_read(core, r5));
			avr_data_write_w(core, 0, (uint16_t)mul);

			if (mul & 0x8000) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);

			if (mul == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			opcycles++;

			if (trace)
				printf("MULSU r%d, r%d", d5, r5);
		}
		else if ((opcode & 0xFF88) == 0x0308) { //FMUL 0000 0011 0ddd 1rrr
			int16_t mul;
			r5 = (opcode & 0x7) + 16;
			d5 = ((opcode >> 4) & 0x7) + 16;
			mul = ((uint8_t)avr_data_read(core, d5)) * ((uint8_t)avr_data_read(core, r5));

			if (mul & 0x8000) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);

			mul <<= 1;
			avr_data_write_w(core, 0, (uint16_t)mul);

			if (mul == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			opcycles++;

			if (trace)
				printf("FMUL r%d, r%d", d5, r5);
		}
		else if ((opcode & 0xFF88) == 0x0380) { //FMULS 0000 0011 1ddd 0rrr
			int16_t mul;
			r5 = (opcode & 0x7) + 16;
			d5 = ((opcode >> 4) & 0x7) + 16;
			mul = ((int8_t)avr_data_read(core, d5)) * ((int8_t)avr_data_read(core, r5));

			if (mul & 0x8000) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);

			mul <<= 1;
			avr_data_write_w(core, 0, (uint16_t)mul);

			if (mul == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			opcycles++;

			if (trace)
				printf("FMULS r%d, r%d", d5, r5);
		}
		else if ((opcode & 0xFF88) == 0x0388) { //FMULSU 0000 0011 1ddd 1rrr
			int16_t mul;
			r5 = (opcode & 0x7) + 16;
			d5 = ((opcode >> 4) & 0x7) + 16;
			mul = ((int8_t)avr_data_read(core, d5)) * ((uint8_t)avr_data_read(core, r5));

			if (mul & 0x8000) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);

			mul <<= 1;
			avr_data_write_w(core, 0, (uint16_t)mul);

			if (mul == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			if (trace)
				printf("FMULSU r%d, r%d", d5, r5);
		}
		else if ((opcode & 0xFC00) == 0x0400) { //CPC 0000 01rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_sub_c(core, avr_data_read(core, d5), avr_data_read(core, r5), GET_SREG_BIT(core, SREG_C));
			if (trace)
				printf("CPC r%d, r%d (%02X)", d5, r5, result);
		}
		else if ((opcode & 0xFC00) == 0x0800) { //SBC 0000 10rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_sub_c(core, avr_data_read(core, d5), avr_data_read(core, r5), GET_SREG_BIT(core, SREG_C));
			avr_data_write(core, d5, result);
			if (trace)
				printf("SBC r%d, r%d (%02X)", d5, r5, result);
		}
		else if ((opcode & 0xFC00) == 0x0C00) { //ADD 0000 11rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_add(core, avr_data_read(core, d5), avr_data_read(core, r5));
			avr_data_write(core, d5, result);
			if (trace)
				printf("ADD r%d, r%d (%02X)", d5, r5, result);
		}
		else { //TODO: Can this even be reached?
			invalid = 1;
		}
		break;

	case 0x1:
		if ((opcode & 0xFC00) == 0x1000) { //CPSE 0001 00rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			if (avr_data_read(core, d5) == avr_data_read(core, r5)) {
				core->skip_next = 1;
			}
			if (trace)
				printf("CPSE d%u, r%u", d5, r5);
		}
		else if ((opcode & 0xFC00) == 0x1400) { //CP 0001 01rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_sub(core, avr_data_read(core, d5), avr_data_read(core, r5));
			if (trace)
				printf("CP r%d, r%d (%02X)", d5, r5, result);
		}
		else if ((opcode & 0xFC00) == 0x1800) { //SUB 0001 10rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_sub(core, avr_data_read(core, d5), avr_data_read(core, r5));
			avr_data_write(core, d5, result);
			if (trace)
				printf("SUB r%d, r%d (%02X)", d5, r5, result);
		}
		else if ((opcode & 0xFC00) == 0x1C00) { //ADC 0001 11rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_add_c(core, avr_data_read(core, d5), avr_data_read(core, r5), GET_SREG_BIT(core, SREG_C));
			avr_data_write(core, d5, result);
			if (trace)
				printf("ADC r%d, r%d (%02X)", d5, r5, result);
		}
		else { //TODO: Can this even be reached?
			invalid = 1;
		}
		break;

	case 0x2:
		if ((opcode & 0xFC00) == 0x2000) { //AND 0010 00rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5) & avr_data_read(core, r5);
			avr_data_write(core, d5, result);
			avr_bitwise_flags(core, result);
			if (trace)
				printf("AND r%d, r%d (%02X)", d5, r5, result);
		}
		else if ((opcode & 0xFC00) == 0x2400) { //EOR 0010 01rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5) ^ avr_data_read(core, r5);
			avr_data_write(core, d5, result);
			avr_bitwise_flags(core, result);
			if (trace)
				printf("EOR r%d, r%d (%02X)", d5, r5, result);
		}
		else if ((opcode & 0xFC00) == 0x2800) { //OR 0010 10rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5) | avr_data_read(core, r5);
			avr_data_write(core, d5, result);
			avr_bitwise_flags(core, result);
			if (trace)
				printf("OR r%d, r%d (%02X)", d5, r5, result);
		}
		else if ((opcode & 0xFC00) == 0x2C00) { //MOV 0010 11rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			avr_data_write(core, d5, result);
			if (trace)
				printf("MOV r%d, r%d (%02X)", d5, r5, result);
		}
		else { //TODO: Can this even be reached?
			invalid = 1;
		}
		break;

	case 0x3: //CPI 0011 kkkk dddd kkkk
		K = (opcode & 0xF) | ((opcode >> 4) & 0xF0);
		r5 = ((opcode >> 4) & 0xF) + 16;
		result = avr_sub(core, avr_data_read(core, r5), K);
		if (trace)
			printf("CPI r%d, %02X (%02X)", r5, K, result);
		break;

	case 0x4: //SBCI 0100 kkkk dddd kkkk TODO: verify this and SUBI
		K = (opcode & 0xF) | ((opcode >> 4) & 0xF0);
		d5 = ((opcode >> 4) & 0xF) + 16;
		result = avr_sub_c(core, avr_data_read(core, d5), K, GET_SREG_BIT(core, SREG_C));
		avr_data_write(core, d5, result);
		if (trace)
			printf("SBCI r%d, %02X (%02X)", d5, K, result);
		break;
	
	case 0x5: //SUBI 0101 kkkk dddd kkkk
		K = (opcode & 0xF) | ((opcode >> 4) & 0xF0);
		d5 = ((opcode >> 4) & 0xF) + 16;
		result = avr_sub(core, avr_data_read(core, d5), K);
		avr_data_write(core, d5, result);
		if (trace)
			printf("SUBI r%d, %02X (%02X)", d5, K, result);
		break;

	case 0x6: //ORI 0110 kkkk dddd kkkk
		K = (opcode & 0xF) | ((opcode >> 4) & 0xF0);
		d5 = ((opcode >> 4) & 0xF) + 16;
		result = avr_data_read(core, d5) | K;
		avr_data_write(core, d5, result);
		avr_bitwise_flags(core, result);
		if (trace)
			printf("ORI r%d, %02X (%02X)", d5, K, result);
		break;

	case 0x7: //ANDI 0111 kkkk dddd kkkk
		K = (opcode & 0xF) | ((opcode >> 4) & 0xF0);
		d5 = ((opcode >> 4) & 0xF) + 16;
		result = avr_data_read(core, d5) & K;
		avr_data_write(core, d5, result);
		avr_bitwise_flags(core, result);
		if (trace)
			printf("ORI r%d, %02X (%02X)", d5, K, result);
		break;

	case 0x8:
	case 0xA:
		if ((opcode & 0xD208) == 0x8000) { //LDD_Z 10q0 qq0d dddd 0qqq
			d5 = (opcode >> 4) & 0x1F;
			q6 = (opcode & 7) | ((opcode >> 7) & 0x18) | ((opcode >> 8) & 0x20);
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			if (core->regaddr[R_RAMPZ]) ea |= (((uint32_t)avr_data_read(core, REG(R_RAMPZ)) & 3) << 16);
			ea += q6;
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			if (trace)
				printf("LDD_Z r%d, %04X (%02X) q6 = %02X", d5, ea, result, q6);
		}
		else if ((opcode & 0xD208) == 0x8200) { //STD_Z 10q0 qq1d dddd 0qqq
			d5 = (opcode >> 4) & 0x1F;
			q6 = (opcode & 7) | ((opcode >> 7) & 0x18) | ((opcode >> 8) & 0x20);
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			if (core->regaddr[R_RAMPZ]) ea |= (((uint32_t)avr_data_read(core, REG(R_RAMPZ)) & 3) << 16);
			ea += q6;
			result = avr_data_read(core, d5);
			avr_data_write(core, ea, result);
			if (trace)
				printf("STD_Z %04X, r%d (%02X)", ea, d5, result);
		}
		else if ((opcode & 0xD208) == 0x8208) { //STD_Y 10q0 qq1d dddd 1qqq
			d5 = (opcode >> 4) & 0x1F;
			q6 = (opcode & 7) | ((opcode >> 7) & 0x18) | ((opcode >> 8) & 0x20);
			ea = (uint32_t)avr_data_read_w(core, REG(R_Y));
			if (core->regaddr[R_RAMPY]) ea |= (((uint32_t)avr_data_read(core, REG(R_RAMPY)) & 3) << 16);
			ea += q6;
			result = avr_data_read(core, d5);
			avr_data_write(core, ea, result);
			if (trace)
				printf("STD_Y %04X, r%d (%02X)", ea, d5, result);
		}
		else if ((opcode & 0xD208) == 0x8008) { //LDD_Y 10q0 qq0d dddd 1qqq
			d5 = (opcode >> 4) & 0x1F;
			q6 = (opcode & 7) | ((opcode >> 7) & 0x18) | ((opcode >> 8) & 0x20);
			ea = (uint32_t)avr_data_read_w(core, REG(R_Y));
			if (core->regaddr[R_RAMPY]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPY)) << 16);
			ea += q6;
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			if (trace)
				printf("LDD_Y r%d, %04X (%02X)", d5, ea, result);
		}
		else { //TODO: Can this even be reached?
			invalid = 1;
		}
		break;

	case 0x9:
		if ((opcode & 0xFE0F) == 0x9000) { //LDS 1001 000d dddd 0000
			d5 = (opcode >> 4) & 0x1F;
			ea = avr_flash_read(core, ++core->pc);
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			opcycles++;
			if (trace)
				printf("LDS r%d, %04X (%02X)", d5, ea, result);
		}
		else if ((opcode & 0xFE0F) == 0x9001) { //LD_Z_inc 1001 000d dddd 0001
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			if (core->regaddr[R_RAMPZ]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPZ)) << 16);
			//ea %= core->data_size;
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			if (trace)
				printf("LD_Z_inc r%u, %08X (%02X)", d5, ea, result);
			ea++;
			avr_data_write_w(core, REG(R_Z), (uint16_t)ea);
			if (core->regaddr[R_RAMPZ]) avr_data_write(core, REG(R_RAMPZ), (uint8_t)(ea >> 16));
		}
		else if ((opcode & 0xFE0F) == 0x9002) { //LD_Z_dec 1001 000d dddd 0010
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			if (core->regaddr[R_RAMPZ]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPZ)) << 16);
			//ea %= core->data_size;
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, --ea);
			avr_data_write(core, d5, result);
			if (trace)
				printf("LD_Z_inc r%u, %08X (%02X)", d5, ea, result);
			avr_data_write_w(core, REG(R_Z), (uint16_t)ea);
			if (core->regaddr[R_RAMPZ]) avr_data_write(core, REG(R_RAMPZ), (uint8_t)(ea >> 16));
		}
		else if ((opcode & 0xFE0F) == 0x9003) { //LD_Z_min 1001 000d dddd 0011
			printf("LD_Z_min\n"); invalid = 1; //TODO: implement LD_Z_min
		}
		else if ((opcode & 0xFFFF) == 0x95C8) { //LPM 1001 0101 1100 1000
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			//ea %= core->flash_size;
			result = avr_flash_read_byte(core, ea);
			avr_data_write(core, 0, result);
			opcycles += 2;
			if (trace)
				printf("LPM r0, %08X (%02X)", ea, result);
		}
		else if ((opcode & 0xFE0F) == 0x9004) { //LPM_Z 1001 000d dddd 0100
			d5 = (opcode >> 4) & 0x1F;
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			//ea %= core->flash_size;
			result = avr_flash_read_byte(core, ea);
			avr_data_write(core, d5, result);
			opcycles += 2;
			if (trace)
				printf("LPM_Z r%d, %08X (%02X)", d5, ea, result);
		}
		else if ((opcode & 0xFE0F) == 0x9005) { //LPM_Z_inc 1001 000d dddd 0101
			d5 = (opcode >> 4) & 0x1F;
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			//ea %= core->flash_size;
			result = avr_flash_read_byte(core, ea);
			if (trace)
				printf("LPM_Z_inc r%d, %08X (%02X)", d5, ea, result);
			avr_data_write(core, d5, result);
			ea++;
			avr_data_write_w(core, REG(R_Z), (uint16_t)ea);
			opcycles += 2;
		}
		else if ((opcode & 0xFFFF) == 0x95D8) { //ELPM 1001 0101 1101 1000
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPZ)) << 16);
			//ea %= core->flash_size;
			result = avr_flash_read_byte(core, ea);
			avr_data_write(core, 0, result);
			opcycles += 2;
			if (trace)
				printf("ELPM r0, %08X (%02X)", ea, result);
		}
		else if ((opcode & 0xFE0F) == 0x9006) { //ELPM_Z 1001 000d dddd 0110
			d5 = (opcode >> 4) & 0x1F;
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPZ)) << 16);
			//ea %= core->flash_size;
			result = avr_flash_read_byte(core, ea);
			avr_data_write(core, d5, result);
			opcycles += 2;
			if (trace)
				printf("ELPM_Z r%d, %08X (%02X)", d5, ea, result);
		}
		else if ((opcode & 0xFE0F) == 0x9007) { //ELPM_Z_inc 1001 000d dddd 0111
			d5 = (opcode >> 4) & 0x1F;
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPZ)) << 16);
			//ea %= core->flash_size;
			result = avr_flash_read_byte(core, ea);
			if (trace)
				printf("ELPM_Z_inc r%d, %08X (%02X)", d5, ea, result);
			avr_data_write(core, d5, result);
			ea++;
			avr_data_write_w(core, REG(R_Z), (uint16_t)ea);
			if (core->regaddr[R_RAMPZ]) avr_data_write(core, REG(R_RAMPZ), (uint8_t)(ea >> 16));
			opcycles += 2;
		}
		else if ((opcode & 0xFE0F) == 0x9009) { //LD_Y_inc 1001 000d dddd 1001
			d5 = (opcode >> 4) & 0x1F;
			ea = avr_data_read_w(core, REG(R_Y));
			if (core->regaddr[R_RAMPY]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPY)) << 16);
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			if (trace)
				printf("LD_Y_inc r%u, %04X (%02X)", d5, ea, result);
			ea++;
			avr_data_write_w(core, REG(R_Y), (uint16_t)ea);
			if (core->regaddr[R_RAMPY]) avr_data_write(core, REG(R_RAMPY), (uint8_t)(ea >> 16));
		}
		else if ((opcode & 0xFE0F) == 0x900A) { //LD_Y_dec 1001 000d dddd 1010
			d5 = (opcode >> 4) & 0x1F;
			ea = avr_data_read_w(core, REG(R_Y));
			if (core->regaddr[R_RAMPY]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPY)) << 16);
			ea--;
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			avr_data_write_w(core, REG(R_Y), (uint16_t)ea);
			if (core->regaddr[R_RAMPY]) avr_data_write(core, REG(R_RAMPY), (uint8_t)(ea >> 16));
			if (trace)
				printf("LD_Y_dec r%u, %04X (%02X)", d5, ea, result);
		}
		else if ((opcode & 0xFE0F) == 0x900C) { //LD_X 1001 000d dddd 1100
			d5 = (opcode >> 4) & 0x1F;
			ea = avr_data_read_w(core, REG(R_X));
			if (core->regaddr[R_RAMPX]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPX)) << 16);
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			if (trace)
				printf("LD_X r%u, %04X (%02X)", d5, ea, result);
		}
		else if ((opcode & 0xFE0F) == 0x900D) { //LD_X_inc 1001 000d dddd 1101
			d5 = (opcode >> 4) & 0x1F;
			ea = avr_data_read_w(core, REG(R_X));
			if (core->regaddr[R_RAMPX]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPX)) << 16);
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			if (trace)
				printf("LD_X_inc r%u, %04X (%02X)", d5, ea, result);
			ea++;
			avr_data_write_w(core, REG(R_X), (uint16_t)ea);
			if (core->regaddr[R_RAMPX]) avr_data_write(core, REG(R_RAMPX), (uint8_t)(ea >> 16));
		}
		else if ((opcode & 0xFE0F) == 0x900E) { //LD_X_dec 1001 000d dddd 1110
			d5 = (opcode >> 4) & 0x1F;
			ea = avr_data_read_w(core, REG(R_X));
			if (core->regaddr[R_RAMPX]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPX)) << 16);
			ea--;
			result = avr_data_read(core, ea);
			avr_data_write(core, d5, result);
			avr_data_write_w(core, REG(R_X), (uint16_t)ea);
			if (core->regaddr[R_RAMPX]) avr_data_write(core, REG(R_RAMPX), (uint8_t)(ea >> 16));
			if (trace)
				printf("LD_X_dec r%u, %04X (%02X)", d5, ea, result);
		}
		else if ((opcode & 0xFE0F) == 0x900F) { //POP 1001 000d dddd 1111
			d5 = (opcode >> 4) & 0x1F;
			result = avr_pop_byte(core);
			avr_data_write(core, d5, result);
			opcycles++;
			if (trace)
				printf("POP r%d (%02X)", d5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9200) { //STS 1001 001r rrrr 0000
			r5 = (opcode >> 4) & 0x1F;
			ea = avr_flash_read(core, ++core->pc); //TODO: RAMPD
			result = avr_data_read(core, r5);
			avr_data_write(core, ea, result);
			opcycles++;
			if (trace)
				printf("STS %04X, r%d (%02X)", ea, r5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9201) { //ST_Z_inc 1001 001r rrrr 0001
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			if (core->regaddr[R_RAMPZ]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPZ)) << 16);
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			avr_data_write(core, ea++, result);
			avr_data_write_w(core, REG(R_Z), (uint16_t)ea);
			if (core->regaddr[R_RAMPZ]) avr_data_write(core, REG(R_RAMPZ), (uint8_t)(ea >> 16));
			if (trace)
				printf("ST_Z_inc %04X, r%d (%02X)", ea, r5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9202) { //ST_Z_dec 1001 001r rrrr 0010
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			if (core->regaddr[R_RAMPZ]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPZ)) << 16);
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			avr_data_write(core, --ea, result);
			avr_data_write_w(core, REG(R_Z), (uint16_t)ea);
			if (core->regaddr[R_RAMPZ]) avr_data_write(core, REG(R_RAMPZ), (uint8_t)(ea >> 16));
			if (trace)
				printf("ST_Y_dec %04X, r%d (%02X)", ea, r5, result);
		}
		else if ((opcode & 0xFE0C) == 0x9204) { //unk1 1001 001r rrrr 01xx
			printf("unk1\n"); invalid = 1; //TODO: implement unk1
		}
		else if ((opcode & 0xFE0F) == 0x9209) { //ST_Y_inc 1001 001r rrrr 1001
			ea = (uint32_t)avr_data_read_w(core, REG(R_Y));
			if (core->regaddr[R_RAMPY]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPY)) << 16);
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			avr_data_write(core, ea++, result);
			avr_data_write_w(core, REG(R_Y), (uint16_t)ea);
			if (core->regaddr[R_RAMPY]) avr_data_write(core, REG(R_RAMPY), (uint8_t)(ea >> 16));
			if (trace)
				printf("ST_Y_inc %04X, r%d (%02X)", ea, r5, result);
		}
		else if ((opcode & 0xFE0F) == 0x920A) { //ST_Y_dec 1001 001r rrrr 1010
			ea = (uint32_t)avr_data_read_w(core, REG(R_Y));
			if (core->regaddr[R_RAMPY]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPY)) << 16);
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			avr_data_write(core, --ea, result);
			avr_data_write_w(core, REG(R_Y), (uint16_t)ea);
			if (core->regaddr[R_RAMPY]) avr_data_write(core, REG(R_RAMPY), (uint8_t)(ea >> 16));
			if (trace)
				printf("ST_Y_dec %04X, r%d (%02X)", ea, r5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9208) { //ST_Y 1001 001r rrrr 1000
			ea = (uint32_t)avr_data_read_w(core, REG(R_Y));
			if (core->regaddr[R_RAMPY]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPY)) << 16);
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			avr_data_write(core, ea, result);
			if (trace)
				printf("ST_Y %04X, r%d (%02X)", ea, r5, result);
		}
		else if ((opcode & 0xFE0F) == 0x920D) { //ST_X_inc 1001 001r rrrr 1101
			ea = (uint32_t)avr_data_read_w(core, REG(R_X));
			if (core->regaddr[R_RAMPX]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPX)) << 16);
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			if (trace)
				printf("ST_X_inc %04X, r%d (%02X)", ea, r5, result);
			avr_data_write(core, ea++, result);
			avr_data_write_w(core, REG(R_X), (uint16_t)ea);
			if (core->regaddr[R_RAMPX]) avr_data_write(core, REG(R_RAMPX), (uint8_t)(ea >> 16));
		}
		else if ((opcode & 0xFE0F) == 0x920E) { //ST_X_dec 1001 001r rrrr 1110
			ea = (uint32_t)avr_data_read_w(core, REG(R_X));
			if (core->regaddr[R_RAMPX]) ea |= ((uint32_t)avr_data_read(core, REG(R_RAMPX)) << 16);
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			if (trace)
				printf("ST_X_dec %04X, r%d (%02X)", ea, r5, result);
			avr_data_write(core, --ea, result);
			avr_data_write_w(core, REG(R_X), (uint16_t)ea);
			if (core->regaddr[R_RAMPX]) avr_data_write(core, REG(R_RAMPX), (uint8_t)(ea >> 16));
		}
		else if ((opcode & 0xFE0F) == 0x920C) { //ST_X 1001 001r rrrr 1100
			ea = (uint32_t)avr_data_read_w(core, REG(R_X)); //TODO: RAMPX
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5);
			avr_data_write(core, ea, result);
			if (trace)
				printf("ST_X %04X, r%d (%02X)", ea, r5, result);
		}
		else if ((opcode & 0xFE0F) == 0x920F) { //PUSH 1001 001d dddd 1111
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5);
			avr_push_byte(core, result);
			opcycles++;
			if (trace)
				printf("PUSH r%d (%02X)", d5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9400) { //COM 1001 010d dddd 0000
			d5 = (opcode >> 4) & 0x1F;
			result = 0xFF - avr_data_read(core, d5);
			avr_data_write(core, d5, result);

			REG_CLR_BIT(core, R_SREG, SREG_V);

			REG_SET_BIT(core, R_SREG, SREG_C);

			if (result & 0x80) REG_SET_BIT(core, R_SREG, SREG_N);
			else REG_CLR_BIT(core, R_SREG, SREG_N);

			if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
			else REG_CLR_BIT(core, R_SREG, SREG_S);

			if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			if (trace)
				printf("COM r%d (%02X)", d5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9401) { //NEG 1001 010d dddd 0001
			d5 = (opcode >> 4) & 0x1F;
			tempval = avr_data_read(core, d5);
			result = 0 - tempval;
			avr_data_write(core, d5, result);

			if ((result | tempval) & 0x08) REG_SET_BIT(core, R_SREG, SREG_H);
			else REG_CLR_BIT(core, R_SREG, SREG_H);

			if (result == 0x80) REG_SET_BIT(core, R_SREG, SREG_V);
			else REG_CLR_BIT(core, R_SREG, SREG_V);

			if (result) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);

			if (result & 0x80) 	REG_SET_BIT(core, R_SREG, SREG_N);
			else REG_CLR_BIT(core, R_SREG, SREG_N);

			if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
			else REG_CLR_BIT(core, R_SREG, SREG_S);

			if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			if (trace)
				printf("NEG r%d (%02X)", d5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9402) { //SWAP 1001 010d dddd 0010
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5);
			result = (result << 4) | (result >> 4);
			avr_data_write(core, d5, result);
			if (trace)
				printf("SWAP r%d, %02X -> %02X", d5, ((result << 4) | (result >> 4)) & 0xFF, result);
		}
		else if ((opcode & 0xFE0F) == 0x9403) { //INC 1001 010d dddd 0011
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5) + 1;
			avr_data_write(core, d5, result);

			if (result == 0x80) REG_SET_BIT(core, R_SREG, SREG_V);
			else REG_CLR_BIT(core, R_SREG, SREG_V);

			if (result & 0x80) REG_SET_BIT(core, R_SREG, SREG_N);
			else REG_CLR_BIT(core, R_SREG, SREG_N);

			if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
			else REG_CLR_BIT(core, R_SREG, SREG_S);

			if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			if (trace)
				printf("INC r%d", d5);
		}
		else if ((opcode & 0xFE0F) == 0x9404) { //unk2 1001 010d dddd 0100
			printf("unk2\n"); invalid = 1; //TODO: implement unk2
		}
		else if ((opcode & 0xFE0F) == 0x9405) { //ASR 1001 010d dddd 0101
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5);

			if (result & 1) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);

			result = (result & 0x80) | (result >> 1);
			if (result & 0x80) REG_SET_BIT(core, R_SREG, SREG_N);
			else REG_CLR_BIT(core, R_SREG, SREG_N);

			if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_C)) REG_SET_BIT(core, R_SREG, SREG_V);
			else REG_CLR_BIT(core, R_SREG, SREG_V);

			if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
			else REG_CLR_BIT(core, R_SREG, SREG_S);

			if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);
			avr_data_write(core, d5, result);
			if (trace)
				printf("ASR r%d (%02X)", d5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9406) { //LSR 1001 010d dddd 0110
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5);
			if (result & 1) { //TODO: is all this right?
				REG_SET_BIT(core, R_SREG, SREG_C);
				REG_SET_BIT(core, R_SREG, SREG_V);
				REG_SET_BIT(core, R_SREG, SREG_S);
			}
			else {
				REG_CLR_BIT(core, R_SREG, SREG_C);
				REG_CLR_BIT(core, R_SREG, SREG_V);
				REG_CLR_BIT(core, R_SREG, SREG_S);
			}
			result >>= 1;
			REG_CLR_BIT(core, R_SREG, SREG_N);
			if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);
			avr_data_write(core, d5, result);
			if (trace)
				printf("LSR r%d (%02X)", d5, result);
		}
		else if ((opcode & 0xFE0F) == 0x9407) { //ROR 1001 010d dddd 0111
			d5 = (opcode >> 4) & 0x1F;
			val16 = avr_data_read(core, d5);
			val16 |= GET_SREG_BIT(core, SREG_C) << 8;
			if (val16 & 1) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);
			val16 >>= 1;
			result = (uint8_t)val16;
			if (result & 0x80) REG_SET_BIT(core, R_SREG, SREG_N);
			else REG_CLR_BIT(core, R_SREG, SREG_N);

			if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_C)) REG_SET_BIT(core, R_SREG, SREG_V);
			else REG_CLR_BIT(core, R_SREG, SREG_V);

			if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
			else REG_CLR_BIT(core, R_SREG, SREG_S);

			if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);
			avr_data_write(core, d5, result);
			if (trace)
				printf("ROR r%d (%02X)", d5, result);
		}
		else if ((opcode & 0xFE0F) == 0x940A) { //DEC 1001 010d dddd 1010
			d5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, d5) - 1;
			avr_data_write(core, d5, result);

			if (result == 0x7F) REG_SET_BIT(core, R_SREG, SREG_V);
			else REG_CLR_BIT(core, R_SREG, SREG_V);

			if (result & 0x80) REG_SET_BIT(core, R_SREG, SREG_N);
			else REG_CLR_BIT(core, R_SREG, SREG_N);

			if (GET_SREG_BIT(core, SREG_N) ^ GET_SREG_BIT(core, SREG_V)) REG_SET_BIT(core, R_SREG, SREG_S);
			else REG_CLR_BIT(core, R_SREG, SREG_S);

			if (result == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			if (trace)
				printf("DEC r%d (%02X)", d5, result);
		}
		else if ((opcode & 0xFF8F) == 0x9408) { //BSET 1001 0100 0sss 1000
			bit = 1 << ((opcode >> 4) & 7);
			result = avr_data_read(core, REG(R_SREG)) | bit;
			avr_data_write(core, REG(R_SREG), result);
			if (trace)
				printf("BSET %u", bit);
		}
		else if ((opcode & 0xFF8F) == 0x9488) { //BCLR 1001 0100 1sss 1000
			bit = 1 << ((opcode >> 4) & 7);
			result = avr_data_read(core, REG(R_SREG)) & ~bit;
			avr_data_write(core, REG(R_SREG), result);
			if (trace)
				printf("BSET %u", bit);
		}
		else if ((opcode & 0xFFFF) == 0x9409) { //IJMP 1001 0100 0000 1001
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			core->pc = ea;
			opcycles++;
			if (trace)
				printf("IJMP %04X", core->pc);
			goto skip_set_pc;
		}
		else if ((opcode & 0xFFFF) == 0x9419) { //EIJMP 1001 0100 0001 1001
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z)) | ((uint32_t)(avr_data_read(core, REG(R_EIND)) & 0x1F) << 16);
			core->pc = ea;
			opcycles++;
			if (trace) {
				printf("EIJMP %08X", ea);
			}
			goto skip_set_pc;
		}
		else if ((opcode & 0xFFFF) == 0x9508) { //RET 1001 0101 0000 1000
			core->pc = avr_pop_addr(core);
			opcycles += 3;
			if (trace)
				printf("RET");
			goto skip_set_pc;
		}
		else if ((opcode & 0xFFFF) == 0x9509) { //ICALL 1001 0101 0000 1001
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z));
			avr_push_addr(core, core->pc + 1);
			core->pc = ea;
			opcycles += 3;
			if (trace)
				printf("ICALL %08X", ea);
			goto skip_set_pc;
		}
		else if ((opcode & 0xFFFF) == 0x9518) { //RETI 1001 0101 0001 1000
			core->pc = avr_pop_addr(core);
			REG_SET_BIT(core, R_SREG, SREG_I);
			opcycles += 3;
			if (trace)
				printf("RETI");
			goto skip_set_pc;
		}
		else if ((opcode & 0xFFFF) == 0x9519) { //EICALL 1001 0101 0001 1001
			ea = (uint32_t)avr_data_read_w(core, REG(R_Z)) | ((uint32_t)(avr_data_read(core, REG(R_EIND)) & 0x1F) << 16);
			avr_push_addr(core, core->pc + 1);
			core->pc = ea;
			opcycles += 3;
			if (trace) {
				printf("EICALL %08X", ea);
			}
			goto skip_set_pc;
		}
		else if ((opcode & 0xFFFF) == 0x9588) { //SLEEP 1001 0101 1000 1000
			core->sleeping = 1;
			if (trace)
				printf("SLEEP");
		}
		else if ((opcode & 0xFFFF) == 0x9598) { //BREAK 1001 0101 1001 1000
			printf("BREAK\n"); invalid = 1; //TODO: implement BREAK
		}
		else if ((opcode & 0xFFFF) == 0x95A8) { //WDR 1001 0101 1010 1000
			//TODO: nothing yet, add watchdog stuff
		}
		else if ((opcode & 0xFFFF) == 0x95E8) { //SPM 1001 0101 1110 1000
			printf("SPM\n"); invalid = 1; //TODO: implement SPM
		}
		else if ((opcode & 0xFFFF) == 0x95F8) { //ESPM 1001 0101 1111 1000
			printf("ESPM\n"); invalid = 1; //TODO: implement ESPM
		}
		else if ((opcode & 0xFE0E) == 0x940C) { //JMP 1001 010k kkkk 110k
			k = avr_flash_read(core, ++core->pc);
			k |= (uint32_t)((opcode & 0x0001) | ((opcode & 0x01F0) >> 3)) << 16;
			core->pc = k;
			opcycles += 2;
			if (trace)
				printf("JMP %08X", k);
			goto skip_set_pc;
		}
		else if ((opcode & 0xFE0E) == 0x940E) { //CALL 1001 010k kkkk 111k
			k = avr_flash_read(core, ++core->pc);
			k |= (uint32_t)((opcode & 0x0001) | ((opcode & 0x01F0) >> 3)) << 16;
			avr_push_addr(core, core->pc + 1);
			core->pc = k;
			opcycles += 5;
			if (trace)
				printf("CALL %08X", k);
			goto skip_set_pc;
		}
		else if ((opcode & 0xFF00) == 0x9600) { //ADIW 1001 0110 kkdd kkkk TODO: is this all right?
			K = (opcode & 0xF) | ((opcode >> 2) & 0x30);
			d5 = (((opcode >> 4) & 3) << 1) + 24;
			val16 = avr_add_w(core, avr_data_read_w(core, d5), K);
			avr_data_write_w(core, d5, val16);
			if (trace)
				printf("ADIW r%d, %u (%04X)", d5, K, val16);
		}
		else if ((opcode & 0xFF00) == 0x9700) { //SBIW 1001 0111 kkdd kkkk
			K = (opcode & 0xF) | ((opcode >> 2) & 0x30);
			d5 = (((opcode >> 4) & 3) << 1) + 24;
			val16 = avr_sub_w(core, avr_data_read_w(core, d5), K);
			avr_data_write_w(core, d5, val16);
			if (trace)
				printf("SBIW r%d, %u (%04X)", d5, K, val16);
		}
		else if ((opcode & 0xFF00) == 0x9800) { //CBI 1001 1000 AAAA Abbb
			bit = 1 << (opcode & 7);
			a5 = ((opcode >> 3) & 0x1F) + 32;
			result = avr_data_read(core, a5) & ~bit;
			avr_data_write(core, a5, result);
			opcycles++;
			if (trace)
				printf("SBI %u, %u", a5, opcode & 7);
		}
		else if ((opcode & 0xFF00) == 0x9900) { //SBIC 1001 1001 AAAA Abbb
			bit = 1 << (opcode & 7);
			a5 = ((opcode >> 3) & 0x1F) + 32;
			if ((avr_data_read(core, a5) & bit) == 0)
				core->skip_next = 1;
			opcycles++;
			if (trace)
				printf("SBIC %u, %u", a5, opcode & 7);
		}
		else if ((opcode & 0xFF00) == 0x9A00) { //SBI 1001 1010 AAAA Abbb
			bit = 1 << (opcode & 7);
			a5 = ((opcode >> 3) & 0x1F) + 32;
			result = avr_data_read(core, a5) | bit;
			avr_data_write(core, a5, result);
			opcycles++;
			if (trace)
				printf("SBI %u, %u", a5, opcode & 7);
		}
		else if ((opcode & 0xFF00) == 0x9B00) { //SBIS 1001 1011 AAAA Abbb
			bit = 1 << (opcode & 7);
			a5 = ((opcode >> 3) & 0x1F) + 32;
			if (avr_data_read(core, a5) & bit)
				core->skip_next = 1;
			opcycles++;
			if (trace)
				printf("SBIC %u, %u", a5, opcode & 7);
		}
		else if ((opcode & 0xFC00) == 0x9C00) { //MUL 1001 11rd dddd rrrr
			r5 = (opcode & 0xF) | ((opcode >> 5) & 0x10);
			d5 = (opcode >> 4) & 0x1F;
			val16 = (uint16_t)avr_data_read(core, d5) * (uint16_t)avr_data_read(core, r5);
			avr_data_write_w(core, 0, val16);

			if (val16 & 0x8000) REG_SET_BIT(core, R_SREG, SREG_C);
			else REG_CLR_BIT(core, R_SREG, SREG_C);

			if (val16 == 0) REG_SET_BIT(core, R_SREG, SREG_Z);
			else REG_CLR_BIT(core, R_SREG, SREG_Z);

			if (trace)
				printf("MUL r%d, r%d", d5, r5);
		}
		else { //TODO: Can this even be reached? Might be able to remove.
			invalid = 1;
		}
		break;

	case 0xB:
		if ((opcode & 0xF800) == 0xB000) { //IN 1011 0AAd dddd AAAA
			r5 = (opcode >> 4) & 0x1F;
			a6 = (opcode & 0xF) | ((opcode >> 5) & 0x30);
			result = avr_io_in(core, a6);
			avr_data_write(core, r5, result);
			if (trace)
				printf("IN r%d, %04X", r5, a6);
		}
		else if ((opcode & 0xF800) == 0xB800) { //OUT 1011 1AAr rrrr AAAA
			r5 = (opcode >> 4) & 0x1F;
			a6 = (opcode & 0xF) | ((opcode >> 5) & 0x30);
			result = avr_data_read(core, r5);
			avr_io_out(core, a6, result);
			if (trace)
				printf("OUT %04X, r%d (%02X)", a6, r5, result);
		}
		break;

	case 0xC: //RJMP 1100 kkkk kkkk kkkk
		k = SIGNEXT12(opcode & 0xFFF);
		core->pc = SIGNEXTPC + k;
		core->pc &= core->addr_mask;
		opcycles++;
		if (trace)
			printf("RJMP");
		break;

	case 0xD: //RCALL 1101 kkkk kkkk kkkk
		k = SIGNEXT12(opcode & 0xFFF);
		avr_push_addr(core, core->pc + 1);
		core->pc = SIGNEXTPC + k;
		core->pc &= core->addr_mask;
		opcycles += 2; //3 on 22-bit PC devices
		if (trace)
			printf("RCALL");
		break;

	case 0xE: //LDI 1110 kkkk dddd kkkk
		K = (opcode & 0xF) | ((opcode >> 4) & 0xF0);
		r5 = ((opcode >> 4) & 0xF) + 16;
		avr_data_write(core, r5, K);
		if (trace)
			printf("LDI r%d, %02X", r5, K);
		break;

	case 0xF:
		if ((opcode & 0xFC00) == 0xF000) { //BRBS 1111 00kk kkkk ksss
			bit = opcode & 7;
			if (GET_SREG_BIT(core, bit)) {
				branchdst = (opcode >> 3) & 0x7F;
				branchdst = SIGNEXT7(branchdst);
				core->pc += branchdst;
				opcycles++;
			}
			if (trace)
				printf("BRBS %u", bit);
		}
		else if ((opcode & 0xFC00) == 0xF400) { //BRBC 1111 01kk kkkk ksss
			bit = opcode & 7;
			if (GET_SREG_BIT(core, bit) == 0) {
				branchdst = (opcode >> 3) & 0x7F;
				branchdst = SIGNEXT7(branchdst);
				core->pc += branchdst;
				opcycles++;
			}
			if (trace)
				printf("BRBC %u", bit);
		}
		else if ((opcode & 0xFE08) == 0xF800) { //BLD 1111 100d dddd 0bbb
			bit = opcode & 7;
			r5 = (opcode >> 4) & 0x1F;
			result = avr_data_read(core, r5) & ~(1 << bit);
			result |= GET_SREG_BIT(core, SREG_T) ? (1 << bit) : 0;
			avr_data_write(core, r5, result);
			if (trace)
				printf("BLD r%d, %u", r5, bit);
		}
		else if ((opcode & 0xFE08) == 0xFA00) { //BST 1111 101d dddd 0bbb
			bit = opcode & 7;
			r5 = (opcode >> 4) & 0x1F;
			if (avr_data_read(core, r5) & (1 << bit)) REG_SET_BIT(core, R_SREG, SREG_T);
			else REG_CLR_BIT(core, R_SREG, SREG_T);
			if (trace)
				printf("BST r%d, %u", r5, bit);
		}
		else if ((opcode & 0xFE08) == 0xFC00) { //SBRC 1111 110r rrrr 0bbb
			bit = opcode & 7;
			r5 = (opcode >> 4) & 0x1F;
			core->skip_next = (avr_data_read(core, r5) & (1 << bit)) ? 0 : 1;
			if (trace)
				printf("SBRC r%d, %u (%s)", r5, bit, core->skip_next ? "Yes" : "No");
		}
		else if ((opcode & 0xFE08) == 0xFE00) { //SBRS 1111 111r rrrr 0bbb
			bit = opcode & 7;
			r5 = (opcode >> 4) & 0x1F;
			core->skip_next = (avr_data_read(core, r5) & (1 << bit)) ? 1 : 0;
			if (trace)
				printf("SBRS r%d, %u (%s)", r5, bit, core->skip_next ? "Yes" : "No");
		}
		else {
			invalid = 1;
		}
		break;

	default:
		invalid = 1;
	}

	if (invalid) {
		int i;
		printf("INVALID INSTRUCTION @ %08X: %04X (", core->pc, opcode);
		for (i = 15; i >= 0; i--) {
			printf("%c", ((opcode >> i) & 1) ? '1' : '0');
			switch (i) {
			case 12:
			case 8:
			case 4:
				printf(" ");
			}
		}
		printf(")\n");
		return 0x80;
	}

	//avr_dbg_check(core, opcode);

skip_instruction:
	core->pc++;

skip_set_pc:
	cycles -= opcycles;

#ifdef ACCURATE_PERIPHERAL_CLOCKS
	periph_clock(core, opcycles);
	module_clock(opcycles);
#endif

	if (trace && !core->sleeping) {
		int i;
		printf("\n    SREG: %c %c %c %c %c %c %c %c",
			GET_SREG_BIT(core, SREG_I) ? 'I' : 'i',
			GET_SREG_BIT(core, SREG_T) ? 'T' : 't',
			GET_SREG_BIT(core, SREG_H) ? 'H' : 'h',
			GET_SREG_BIT(core, SREG_S) ? 'S' : 's',
			GET_SREG_BIT(core, SREG_V) ? 'V' : 'v',
			GET_SREG_BIT(core, SREG_N) ? 'N' : 'n',
			GET_SREG_BIT(core, SREG_Z) ? 'Z' : 'z',
			GET_SREG_BIT(core, SREG_C) ? 'C' : 'c'
			);
		printf("\n");
		for (i = 0; i < 8; i++) {
			printf("    R%02d: %02X", i, avr_data_read(core, i));
		}
		printf("\n");
		for (i = 8; i < 16; i++) {
			printf("    R%02d: %02X", i, avr_data_read(core, i));
		}
		printf("\n");
		for (i = 16; i < 24; i++) {
			printf("    R%02d: %02X", i, avr_data_read(core, i));
		}
		printf("\n");
		for (i = 24; i < 32; i++) {
			printf("    R%02d: %02X", i, avr_data_read(core, i));
		}
		printf("\n");
		printf("    X = %04X    Y = %04X    Z = %04X    SP: %04X\n\n",
			avr_data_read_w(core, REG(R_X)), avr_data_read_w(core, REG(R_Y)),
			avr_data_read_w(core, REG(R_Z)), avr_data_read_w(core, REG(R_SP)));
	}

	if (cycles > 0) goto execute;

	return cycles;
}

int avr_interrupt(struct avr_core_s* core, uint32_t vector) {
	if (GET_SREG_BIT(core, SREG_I) == 0) return 0; //don't do it if GIE is cleared
	if (core->skip_next) return 0; //don't do it if we're in the middle of skipping an instruction either

	avr_push_addr(core, core->pc);
	REG_CLR_BIT(core, R_SREG, SREG_I);
	core->pc = vector;
	core->sleeping = 0;

	//printf("interrupt vector %02X\n", vector);

	return 1;
}

void avr_dbg_check(struct avr_core_s* core, uint16_t op) {
	uint8_t dbg_regs[32];
	uint8_t dbg_flags;
	uint16_t dbg_op;
	int i, stop;

	if (dbg == NULL) return;

	stop = 0;

	fread(&dbg_op, 2, 1, dbg);
	for (i = 0; i < 32; i++) {
		fread(&dbg_regs[i], 1, 1, dbg);
	}
	fread(&dbg_flags, 1, 1, dbg);

	if (op != dbg_op) {
		printf("\n\nDEBUG ERROR: Opcode %04X does not match trace file! (%04X)\n", op, dbg_op);
		stop = 1;
	}

	for (i = 0; i < 32; i++) {
		if (core->data[i] != dbg_regs[i]) {
			printf("\n\nDEBUG ERROR: Register %d value does not match trace file! (%02X != %02X)\n", i, core->data[i], dbg_regs[i]);
			stop = 1;
		}
	}

	if (core->data[REG(R_SREG)] != dbg_flags) {
		printf("\n\nDEBUG ERROR: Status flags %02X do not match trace file! (%02X)\n", core->data[REG(R_SREG)], dbg_flags);
		stop = 1;
	}

	if (stop) {
		exit(0);
	}
}
