#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <algorithm>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#define SCALE 4
#define idx(x, y) (y * 160 + x)

class GB;

class PPU
{
	GB* gb;
	std::vector<uint8_t> vram;
	std::vector<uint8_t> oam;

	std::vector<uint8_t> framebuf;

	struct ObjEntry {
		uint8_t x;
		uint8_t y;
		uint8_t tileid;
		uint8_t flags;
	};

	std::vector<ObjEntry> active_obj;
	
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
	uint8_t obp0;
	uint8_t obp1;

	enum Mode : uint8_t {
		HBLANK,
		VBLANK,
		OAM_SCAN,
		PIXEL_TRANSFER
	} mode;

	int dot;
	uint8_t lx;

	uint8_t dma;

	void cmp_lyc_ly();
	void check_stat_int();

	void dma_transfer();
	void oam_scan();
	void render_sprites();

	uint16_t get_tile(uint8_t off);
public:
	SDL_Window* win;
	SDL_Renderer* ren;
	SDL_Texture* tex;
	
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
	void reset();
};


