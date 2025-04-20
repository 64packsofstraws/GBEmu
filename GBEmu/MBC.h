#pragma once
#include <cstdint>
#include <vector>

class MBC
{
protected:
	std::vector<uint8_t> rom;
	std::vector<uint8_t> ram;

public:
	MBC(std::vector<uint8_t> rom, std::vector<uint8_t> ram);

	virtual uint8_t cart_read(uint16_t addr) = 0;
	virtual void cart_write(uint16_t addr, uint8_t val) = 0;
};

class MBC0 : public MBC
{
public:
	MBC0(std::vector<uint8_t> rom, std::vector<uint8_t> ram);

	uint8_t cart_read(uint16_t addr);
	void cart_write(uint16_t addr, uint8_t val);
};

class MBC1 : public MBC
{
	bool ram_enable;
	bool mode_sel;
	uint8_t rom_bank_num;
	uint8_t ram_bank_num;

	uint8_t rom_bank_size;
public:
	MBC1(std::vector<uint8_t> rom, std::vector<uint8_t> ram);

	uint8_t cart_read(uint16_t addr);
	void cart_write(uint16_t addr, uint8_t val);
};