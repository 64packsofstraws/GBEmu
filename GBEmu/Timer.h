#pragma once
#include <cstdint>

class GB;

class Timer
{
	uint16_t div;

	uint8_t tima;
	uint16_t tima_cycles;
	uint8_t tma;
	uint8_t tac;

	uint16_t tima_clock_sel;
	bool tima_enable;
	bool delay;
	GB* gb;

	uint8_t get_tac_idx(uint8_t sel);
	void tima_tick();
public:
	Timer(GB* gb);

	void tick();

	uint8_t read_timer(uint16_t addr);
	void write_timer(uint16_t addr, uint8_t val);
};