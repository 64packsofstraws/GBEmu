#pragma once
#include <cstdint>
#include <vector>
#include <iostream>

#define AF ((A << 8) | F)
#define BC ((B << 8) | C)
#define DE ((D << 8) | E)
#define HL ((H << 8) | L)

class GB;

class CPU
{
	uint8_t A;
	uint8_t B, C;
	uint8_t D, E;
	uint8_t H, L;

	bool Z : 1;
	bool N : 1;
	bool HC : 1;
	bool CY : 1;

	uint16_t SP;
	uint16_t PC;

	uint8_t intf;
	uint8_t ie;

	bool ime;

	std::vector<uint8_t> wram;
	std::vector<uint8_t> hram;

	GB* gb;

	void tick_components(uint8_t m);

	uint8_t* get_register(uint8_t n);

	uint8_t prefix_cb();

	void halt();

	void ld(uint8_t& reg1, uint8_t& reg2, uint16_t val);
	void ld(uint8_t& reg, uint8_t val);
	void ld(uint16_t addr, uint8_t val);

	void ldi(uint16_t addr, uint8_t val);
	void ldd(uint16_t addr, uint8_t val);
	void ldi(uint8_t& reg, uint8_t val);
	void ldd(uint8_t& reg, uint8_t val);

	void ld_sp(uint16_t addr);
	void ld_hl_sp();

	void inc(uint8_t& reg1, uint8_t& reg2);
	void inc(uint8_t& reg);
	void inc_hl();

	void dec(uint8_t& reg1, uint8_t& reg2);
	void dec(uint8_t& reg);
	void dec_hl();

	void add(uint8_t& reg1, uint8_t& reg2, uint16_t val);
	void add(uint8_t val);
	void add_sp_e(int8_t val);

	void sub(uint8_t val);

	void adc(uint8_t val);
	void sbc(uint8_t val);

	void anda(uint8_t val);
	void xora(uint8_t val);
	void ora(uint8_t val);

	void cp(uint8_t val);

	void rlc(uint8_t& reg);
	void rlca();

	void rl(uint8_t& reg);
	void rla();

	void rrc(uint8_t& reg);
	void rrca();

	void rr(uint8_t& reg);
	void rra();

	void sla(uint8_t& reg);

	void sra(uint8_t& reg);

	void srl(uint8_t& reg);

	void swap(uint8_t& reg);

	void bit(uint8_t b, uint8_t val);

	void res(uint8_t b, uint8_t& reg);

	void set(uint8_t b, uint8_t& reg);

	void jp(uint16_t addr);
	void jp_cond(bool cond, uint16_t addr);

	void jr(uint8_t off);
	void jr_cond(bool cond, uint8_t off);

	void call(uint16_t addr);
	void call_cond(bool cond, uint16_t addr);

	void ret();
	void ret_cond(bool cond);

	void push(uint16_t val);
	void push_af();

	void pop(uint8_t& reg1, uint8_t& reg2);
	void pop_af();

	void daa();

	void cpl();
	void ccf();
	void scf();

	uint8_t cpu_read(uint16_t addr);
	void cpu_write(uint16_t addr, uint8_t val);

	uint8_t next_byte();
	uint16_t next_word();


	uint8_t handle_int();
public:
	CPU(GB* gb);

	uint8_t read_wram(uint16_t addr);
	void write_wram(uint16_t addr, uint8_t val);

	uint8_t read_hram(uint16_t addr);
	void write_hram(uint16_t addr, uint8_t val);

	uint8_t read_intf();
	void write_intf(uint8_t val);
	void req_intf(uint8_t bit);

	uint8_t read_ie();
	void write_ie(uint8_t val);

	void step();
	void reset();
};