#pragma once
#include <cstdint>
#include <vector>
#include <chrono>

using std::chrono::system_clock;

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
	MBC1(std::vector<uint8_t> _rom, std::vector<uint8_t> _ram);

	uint8_t cart_read(uint16_t addr);
	void cart_write(uint16_t addr, uint8_t val);
};

class MBC3 : public MBC
{
	bool ram_rtc_enable;
	uint8_t rom_bank_num;
	uint8_t ram_rtc_num;
	uint8_t rom_bank_size;
	
	system_clock::time_point tpoint;
	uint64_t rtc_base;
	
	uint8_t rtc_s;
	uint8_t rtc_m;
	uint8_t rtc_h;
	uint16_t rtc_day;
public:
	MBC3(std::vector<uint8_t> _rom, std::vector<uint8_t> _ram);

	uint8_t cart_read(uint16_t addr);
	void cart_write(uint16_t addr, uint8_t val);
};

class MBC5 : public MBC
{
	bool ram_enable;
	uint8_t hi_rom_bank_num;
	uint8_t lo_rom_bank_num;
	uint8_t ram_bank_num;

	uint8_t rom_bank_size;
public:
	MBC5(std::vector<uint8_t> _rom, std::vector<uint8_t> _ram);

	uint8_t cart_read(uint16_t addr);
	void cart_write(uint16_t addr, uint8_t val);
};