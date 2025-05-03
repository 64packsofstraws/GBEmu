#include "PPU.h"
#include "GB.h"

PPU::PPU(GB* gb) : gb(gb), vram(8192, 0), oam(0xA0, 0), framebuf(160 * 144, 0)
{
	lcdc = 0x91;
	stat = 0x84;
	lyc = ly = lx = 0;
	scy = scx = 0;
	wx = wy = wly = 0;
	mode = OAM_SCAN;
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
				mode = HBLANK;
				stat &= (~0x3);
			}

			if ((val & 0x80) && !(lcdc & 0x80)) {
				mode = OAM_SCAN;
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
	return (mode != 3) ? vram[addr - 0x8000] : 0xFF;
}

void PPU::write_vram(uint16_t addr, uint8_t val)
{
	if (mode != 3) vram[addr - 0x8000] = val;
}

uint8_t PPU::read_oam(uint16_t addr)
{
	return (mode != 2 && mode != 3) ? oam[addr - 0xFE00] : 0xFF;
}

void PPU::write_oam(uint16_t addr, uint8_t val)
{
	if (mode != 2 && mode != 3) oam[addr - 0xFE00] = val;
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

	for (int i = 0; i <= 0x9F; i++)
		oam[i] = gb->mmu.read(src + i);
}

void PPU::oam_scan()
{
	ObjEntry entry;

	for (int i = 0; i < 0xA0; i += 4) {
		entry.y = oam[i];
		entry.x = oam[i + 1];
		entry.tileid = oam[i + 2];
		entry.flags = oam[i + 3];

		uint8_t obj_size = (lcdc & 0x4) ? 16 : 8;

		if (ly + 16 >= entry.y && ly + 16 < entry.y + obj_size) {
			active_obj.push_back(entry);
			if (active_obj.size() == 10) break;
		}
	}

	std::sort(active_obj.begin(), active_obj.end(), [](const ObjEntry& a, const ObjEntry& b)
	{
		return a.x > b.x;
	});
}

void PPU::render_sprites()
{
	if (!(lcdc & 0x2)) {
		active_obj.clear();
		return;
	}

	for (auto& i : active_obj) {
		if (i.x == 0 || i.x >= 168) continue;

		uint8_t tileid;
		uint8_t x = i.x - 8;
		uint8_t y = i.y - 16;
		
		if (lcdc & 0x4) {
			tileid = (ly - y >= 8) ? i.tileid | 0x1 : i.tileid & 0xFE;
		}
		else {
			tileid = i.tileid;
		}

		uint8_t ydir = (i.flags & 0x40) ? 7 - (ly - y) : (ly - y);

		uint16_t tileoff = tileid * 16 + 2 * (ydir % 8);

		uint8_t p1 = vram[tileoff];
		uint8_t p2 = vram[tileoff + 1];

		for (int j = 0; j < 8 && x + j < 160; j++) {
			uint8_t xdir = (i.flags & 0x20) ? 0x1 << j : 0x80 >> j;

			bool b1 = (p1 & xdir) != 0;
			bool b2 = (p2 & xdir) != 0;
			
			uint8_t obj_id = (b2 << 1) | b1;
			uint8_t bg_id = framebuf[idx(x + j, ly)];

			if (obj_id) {
				if (!(i.flags & 0x80)) {
					framebuf[idx(x + j, ly)] = obj_id;
				}
				else {
					if (bg_id == 0) framebuf[idx(x + j, ly)] = obj_id;
				}
			}
		}
	}
	active_obj.clear();
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
		case OAM_SCAN:
			if (dot == 80) {
				mode = PIXEL_TRANSFER;
				dot = 0;
				stat = (stat & ~0x3) | mode;
				oam_scan();
			}
			break;

		case PIXEL_TRANSFER:
			if (dot >= 12 && lx < 160) {
				uint16_t tilemap_base;
				uint8_t x, y;

				if (!(lcdc & 1)) {
					framebuf[idx(lx, ly)] = 0;
					lx++;

					if (dot == 172) {
						mode = HBLANK;
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

				bool b1 = ( p1 & ( 0x80 >> (x % 8) )) != 0;
				bool b2 = ( p2 & ( 0x80 >> (x % 8) )) != 0;

				framebuf[idx(lx, ly)] = (b2 << 1) | b1;

				lx++;
			}

			if (dot == 172) {
				mode = HBLANK;
				dot = 0;
				render_sprites();
				check_stat_int();
			}
			break;

		case HBLANK:
			if (dot == 204) {
				wly += (wx < 167 && wy < 144);
				dot = 0;
				lx = 0;

				if (ly == 143) {
					mode = VBLANK;
					wly = 0;
					frame_ready = true;

					gb->cpu.req_intf(0);
					check_stat_int();
				}
				else {
					mode = OAM_SCAN;
					check_stat_int();
				}

				ly++;
				cmp_lyc_ly();
			}
			break;

		case VBLANK:
			if (dot == 456) {
				if (ly == 153) {
					ly = 0;
					lx = 0;
					mode = OAM_SCAN;
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
			uint8_t id = framebuf[idx(x, y)];

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