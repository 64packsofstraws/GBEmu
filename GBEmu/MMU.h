#pragma once
#include <cstdint>

class GB;

class MMU
{
	GB* gb;
public:
	MMU(GB* gb);

	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t val);
};
