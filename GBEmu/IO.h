#pragma once
#include <cstdint>
class GB;

class IO
{
	GB* gb;
public:
	IO(GB* gb);

	uint8_t io_read(uint16_t addr);
	void io_write(uint16_t addr, uint8_t val);
};


