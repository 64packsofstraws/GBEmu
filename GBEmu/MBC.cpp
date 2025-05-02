#include "MBC.h"

MBC::MBC(std::vector<uint8_t> _rom, std::vector<uint8_t> _ram) : rom(std::move(_rom)), ram(std::move(_ram))
{
}

MBC0::MBC0(std::vector<uint8_t> _rom, std::vector<uint8_t> _ram) : MBC(std::move(_rom), std::move(_ram))
{
}

uint8_t MBC0::cart_read(uint16_t addr) {
	return rom[addr];
}

void MBC0::cart_write(uint16_t addr, uint8_t val) {
	// you cant write data to rom, dumbass
}

MBC1::MBC1(std::vector<uint8_t> _rom, std::vector<uint8_t> _ram) : MBC(std::move(_rom), std::move(_ram))
{
	ram_enable = mode_sel = false;
	ram_bank_num = 0;
	rom_bank_num = 1;

	rom_bank_size = rom.size() / 0x4000;
}

uint8_t MBC1::cart_read(uint16_t addr)
{
	if (addr >= 0x0 && addr <= 0x3FFF) {
		return rom[addr];
	}
	else if (addr >= 0x4000 && addr <= 0x7FFF) {
		int idx = (0x4000 * rom_bank_num) + (addr - 0x4000);
		return rom[idx];
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF && ram_enable) {
		int idx = (0x2000 * ram_bank_num) + (addr - 0xA000);
		return ram[idx];
	}

	return 0xFF;
}

void MBC1::cart_write(uint16_t addr, uint8_t val)
{
	if (addr >= 0x0 && addr <= 0x1FFF) {
		ram_enable = (val & 0xF) == 0xA;
	}
	else if (addr >= 0x2000 && addr <= 0x3FFF) {
		if (val == 0x20 || val == 0x40 || val == 0x60) {
			rom_bank_num++;
			return;
		}
		rom_bank_num = ((val & 0x1F) == 0) ? 1 : val & 0x1F;

		if (rom_bank_num >= rom_bank_size)
			rom_bank_num &= rom_bank_size - 1;
	}
	else if (addr >= 0x4000 && addr <= 0x5FFF) {
		ram_bank_num = (val >> 5) & 0x3;
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF && ram_enable) {
		int idx = (0x2000 * ram_bank_num) + (addr - 0xA000);
		ram[idx] = val;
	}
}

MBC3::MBC3(std::vector<uint8_t> _rom, std::vector<uint8_t> _ram) : MBC(std::move(_rom), std::move(_ram))
{
	rom_bank_num = 1;
	ram_rtc_num = 0;
	ram_rtc_enable = false;

	rom_bank_size = rom.size() / 0x4000;

	tpoint = system_clock::now();
	rtc_base = rtc_s = rtc_m = rtc_h = rtc_day = 0;
}

uint8_t MBC3::cart_read(uint16_t addr)
{
	if (addr >= 0x0 && addr <= 0x3FFF) {
		return rom[addr];
	}
	else if (addr >= 0x4000 && addr <= 0x7FFF) {
		int idx = (0x4000 * rom_bank_num) + (addr - 0x4000);
		return rom[idx];
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF && ram_rtc_enable) {
		if (ram_rtc_num < 0x8) {
			int idx = (0x2000 * ram_rtc_num) + (addr - 0xA000);
			return ram[idx];

		}
		else {
			switch (ram_rtc_num) {
				case 0x8: return rtc_s;
				case 0x9: return rtc_m;
				case 0xA: return rtc_h;
				case 0xB: return rtc_day & 0xFF;
				case 0xC: return rtc_day >> 8;
			}
		}
	}

	return 0xFF;
}

void MBC3::cart_write(uint16_t addr, uint8_t val)
{
	if (addr >= 0x0 && addr <= 0x1FFF) {
		ram_rtc_enable = (val & 0xF) == 0xA;
	}
	else if (addr >= 0x2000 && addr <= 0x3FFF) {
		rom_bank_num = ((val & 0x7F) == 0) ? 1 : val & 0x7F;

		if (rom_bank_num >= rom_bank_size)
			rom_bank_num &= rom_bank_size - 1;
	}
	else if (addr >= 0x4000 && addr <= 0x5FFF) {
		ram_rtc_num = val & 0xF;
	}
	else if (addr >= 0x6000 && addr <= 0x7FFF) {
		static uint8_t prev_byte = 0;

		if (prev_byte == 0 && val == 1 && !((rtc_day >> 15) & 1)) {
			auto elapsed = system_clock::now() - tpoint;
			rtc_base += elapsed.count();
			uint64_t tmp = rtc_base;
			uint16_t prev_rtc_day = rtc_day & 0x1FF;

			rtc_day = (rtc_day & (~0x1FF)) | (tmp / 86400) & 0x1FF;
			
			tmp %= 86400;
			rtc_h = tmp / 3600;

			tmp %= 3600;
			rtc_m = tmp / 60;

			tmp %= 60;
			rtc_s = tmp;

			if (prev_rtc_day == 0x1FF && (rtc_day & 0x1FF) == 0) rtc_day |= (1 << 15);
		}

		prev_byte = val;
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (ram_rtc_num < 0x8) {
			int idx = (0x2000 * ram_rtc_num) + (addr - 0xA000);
			ram[idx] = val;
		}
		else {
			switch (ram_rtc_num) {
			case 0x8: rtc_s = val; break;
			case 0x9: rtc_m = val; break;
			case 0xA: rtc_h = val; break;
			case 0xB:
				rtc_day &= 0xFF00;
				rtc_day |= val;
				break;
			case 0xC:
				rtc_day &= 0xFF;
				rtc_day |= (val << 8);
				break;
			}
		}
	}
}
