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
	// fuck you
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
		rom_bank_num = ((val & 0x1F) == 0) ? 1 : val & 0x1F;

		if (rom_bank_num >= rom_bank_size)
			rom_bank_num &= rom_bank_size - 1;
	}
	else if (addr >= 0x4000 && addr <= 0x5FFF) {
		ram_bank_num = val >> 5;
	}
}
