#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <list>
#include <SDL3/SDL.h>

#define SCALE 4
#define idx(x, y) (y * 160 + x)

class GB;

class PPU
{
	GB* gb;
	std::vector<uint8_t> vram;
	std::vector<uint8_t> oam;

	std::vector<uint8_t> pixelbuf;
	std::vector<uint8_t> active_obj;

	uint8_t lcdc;
	uint8_t ly;
	uint8_t lyc;
	uint8_t stat;

	uint8_t scy;
	uint8_t scx;
	uint8_t wy;
	uint8_t wx;
	uint8_t wly;

	uint8_t bgp;

	uint8_t mode;

	int dot;
	uint8_t lx;

	uint8_t dma;

	SDL_Window* win;
	SDL_Renderer* ren;

	void cmp_lyc_ly();
	void check_stat_int();

	void dma_transfer();

	uint16_t get_tile(uint8_t off);
public:
	bool frame_ready;

	PPU(GB* gb);
	~PPU();

	uint8_t ioread(uint16_t addr);
	void iowrite(uint16_t addr, uint8_t val);

	uint8_t read_vram(uint16_t addr);
	void write_vram(uint16_t addr, uint8_t val);

	uint8_t read_oam(uint16_t addr);
	void write_oam(uint16_t addr, uint8_t val);

	void tick();
	void render();
};


