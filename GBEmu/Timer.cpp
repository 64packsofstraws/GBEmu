#include "Timer.h"
#include "GB.h"

uint8_t Timer::get_tac_idx(uint8_t sel)
{
	switch (tac & 0x3) {
		case 0: return 9;
		case 1: return 3;
		case 2: return 5;
		case 3: return 7;
	}
	return 0;
}

void Timer::tima_tick()
{
	if (tima == 0xFF) {
		tima = 0;
		delay = true;

		gb->cpu.req_intf(2);
		return;
	}

	tima++;
}

Timer::Timer(GB* gb) : gb(gb)
{
	tima = tma = 0;
	div = tima_cycles = 0;
	tima_clock_sel = 0;
	tima_enable = delay = false;
	div = 0xAB00;
	tac = 0xF8;
}

void Timer::tick()
{
	uint8_t prev_bit, cur_bit;

	prev_bit = (div >> get_tac_idx(tac & 0x3)) & 1;
	div += 4;
	cur_bit = (div >> get_tac_idx(tac & 0x3)) & 1;

	if (delay && tima == 0) {
		tima = tma;
		delay = false;
		return;
	}

	if (tima_enable && (prev_bit == 1 && cur_bit == 0))
		tima_tick();
}

uint8_t Timer::read_timer(uint16_t addr)
{
	switch (addr) {
		case 0xFF04: return div >> 8;
		case 0xFF05: return tima;
		case 0xFF06: return tma;
		case 0xFF07: return tac;
	}

	return 0xFF;
}

void Timer::write_timer(uint16_t addr, uint8_t val)
{
	switch (addr) {
		case 0xFF04: {
			uint8_t prev_bit;

			prev_bit = (div >> get_tac_idx(tac & 0x3)) & 1;

			div = 0;

			if (tima_enable && prev_bit == 1)
				tima_tick();
		}
				   break;

		case 0xFF05: tima = val; break;
		case 0xFF06: tma = val; break;

		case 0xFF07: {
			uint8_t prev_bit, cur_bit;

			prev_bit = (div >> get_tac_idx(tac & 0x3)) & 1;
			tac = val & 0x7;
			cur_bit = (div >> get_tac_idx(tac & 0x3)) & 1;

			tima_enable = (tac >> 2) & 1;

			if (tima_enable && (cur_bit == 0 && prev_bit == 1))
				tima_tick();
		}
		break;
	}
}