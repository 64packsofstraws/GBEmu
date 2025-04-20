#include "PPU.h"
#include "GB.h"

PPU::PPU(GB* gb) : gb(gb), vram(8192, 0), oam(0xA0, 0), pixelbuf(160 * 144, 0)
{
	lcdc = 0x91;
	stat = 0x84;
	lyc = ly = lx = 0;
	scy = scx = 0;
	wx = wy = wly = 0;
	mode = 2;
	dot = 0;
	bgp = 0;
	dma = 0;
	frame_ready = false;

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer("GBEmu", 160 * SCALE, 144 * SCALE, 0, &win, &ren);
}

PPU::~PPU()
{
	SDL_DestroyWindow(win);
	SDL_DestroyRenderer(ren);
	SDL_Quit();
}

uint8_t PPU::ioread(uint16_t addr)
{
	switch (addr) {
		case 0xFF40: return lcdc;
		case 0xFF41: return stat;
		case 0xFF42: return scy;
		case 0xFF43: return scx;
		case 0xFF44: return ly;
		case 0xFF45: return lyc;
		case 0xFF56: return dma;
		case 0xFF47: return bgp;
		case 0xFF4A: return wy;
		case 0xFF4B: return wx;
	}
	return 0xFF;
}

void PPU::iowrite(uint16_t addr, uint8_t val)
{
	switch (addr) {
		case 0xFF40:
			if (!(val & 1)) val &= ~(1 << 5);

			if (!(val & 0x80)) {
				lx = ly = dot = 0;
				mode = 0;
				stat &= (~0x3);
			}

			if ((val & 0x80) && !(lcdc & 0x80)) {
				mode = 2;
				cmp_lyc_ly();
				check_stat_int();
			}

			lcdc = val;
			break;

		case 0xFF41: stat = val; break;
		case 0xFF42: scy = val; break;
		case 0xFF43: scx = val; break;
		case 0xFF45: lyc = val; cmp_lyc_ly();  break;
		case 0xFF46: dma = val; dma_transfer(); break;
		case 0xFF47: bgp = val; break;
		case 0xFF4A: wy = val; break;
		case 0xFF4B: wx = val; break;
	}
}

uint8_t PPU::read_vram(uint16_t addr)
{
	return vram[addr - 0x8000];
}

void PPU::write_vram(uint16_t addr, uint8_t val)
{
	vram[addr - 0x8000] = val;
}

uint8_t PPU::read_oam(uint16_t addr)
{
	return oam[addr - 0xFE00];
}

void PPU::write_oam(uint16_t addr, uint8_t val)
{
	oam[addr - 0xFE00] = val;
}

void PPU::cmp_lyc_ly()
{
	if (lyc == ly) {
		stat |= (1 << 2);
		if ((stat >> 6) & 1) gb->cpu.req_intf(1);
		return;
	}

	if (lcdc & 0x80) stat &= ~(1 << 2);
}

void PPU::check_stat_int()
{
	stat = (stat & ~(0x3)) | mode;
	if ((stat >> (3 + mode)) & 1) gb->cpu.req_intf(1);
}

void PPU::dma_transfer()
{
	uint16_t src = dma << 8;

	for (int i = 0; i < 0xA0; i++)
		oam[i] = gb->mmu.read(src + i);
}

uint16_t PPU::get_tile(uint8_t off)
{
	return (lcdc & 0x10) ? off * 16 : 0x1000 + static_cast<int8_t>(off) * 16;
}

void PPU::tick()
{
	if (!(lcdc & 0x80)) return;
	dot++;

	switch (mode) {
		case 2:
			if (dot >= 80) {
				mode = 3;
				dot = 0;
				stat = (stat & ~0x3) | mode;
			}
			break;

		case 3:
			if (dot >= 12 && lx < 160) {
				uint16_t tilemap_base;
				uint8_t x, y;

				if (!(lcdc & 1)) {
					pixelbuf[idx(lx, ly)] = 0;
					lx++;

					if (dot >= 172) {
						mode = 0;
						dot = 0;

						check_stat_int();
					}
					return;
				}

				if ((lcdc >> 5) & 1 && (wx < 167 && wy < 144) && ((lx + 7) >= wx && ly >= wy)) {
					tilemap_base = ((lcdc >> 6) & 1) ? 0x1C00 : 0x1800;
					x = lx - (wx - 7);
					y = wly - wy;
				}
				else {
					tilemap_base = ((lcdc >> 3) & 1) ? 0x1C00 : 0x1800;
					x = (scx + lx) % 256;
					y = (scy + ly) % 256;
				}

				uint16_t tmap_idx = tilemap_base + ((y / 8) * 32 + (x / 8));

				uint16_t idx = get_tile(vram[tmap_idx]) + 2 * (y % 8);

				uint8_t p1 = vram[idx];
				uint8_t p2 = vram[idx + 1];

				uint8_t b1 = (p1 & (1 << ( 7 - (x % 8) ))) != 0;
				uint8_t b2 = (p2 & (1 << ( 7 - (x % 8) ))) != 0;

				pixelbuf[idx(lx, ly)] = (b2 << 1) | b1;

				lx++;
			}

			if (dot >= 172) {
				mode = 0;
				dot = 0;

				check_stat_int();
			}
			break;

		case 0:
			if (dot >= 204) {
				wly += (wx < 167 && wy < 144);
				dot = 0;
				lx = 0;

				if (ly == 143) {
					mode = 1;
					wly = 0;
					frame_ready = true;

					gb->cpu.req_intf(0);
					check_stat_int();
				}
				else {
					mode = 2;
					check_stat_int();
				}

				ly++;
				cmp_lyc_ly();
			}
			break;

		case 1:
			if (dot >= 456) {
				if (ly == 153) {
					ly = 0;
					lx = 0;
					mode = 2;
					dot = 0;

					cmp_lyc_ly();
					check_stat_int();
					return;
				}

				ly++;
				cmp_lyc_ly();
				dot = 0;
			}
			break;
	}
}

void PPU::render()
{
	std::map<uint8_t, SDL_Color> id2rgb = {
		{0, {0xFF, 0xFF, 0xFF, 0xFF}},
		{1, {0xA9, 0xA9, 0xA9, 0xFF}},
		{2, {0x54, 0x54, 0x54, 0xFF}},
		{3, {0x00, 0x00, 0x00, 0xFF}}
	};

	SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
	SDL_RenderClear(ren);
	SDL_FRect rect = {0.0, 0.0, SCALE, SCALE};

	for (int y = 0; y < 144; y++) {
		for (int x = 0; x < 160; x++) {
			uint8_t id = pixelbuf[idx(x, y)];

			SDL_Color rgb = id2rgb[(bgp >> (id * 2)) & 0x3];

			SDL_SetRenderDrawColor(ren, rgb.r, rgb.g, rgb.b, rgb.a);
			SDL_RenderFillRect(ren, &rect);

			rect.x += SCALE;
		}
		rect.x = 0;
		rect.y += SCALE;
	}


	SDL_RenderPresent(ren);
	frame_ready = false;
}