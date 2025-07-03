#include "PPU.h"
#include "GB.h"

PPU::PPU(GB* gb) : gb(gb), vram(0x4000, 0), oam(0xA0, 0), framebuf(160 * 144, 0xFFFFFFFF)
{
	reset();

	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer("GBEmu", 160 * SCALE, 144 * SCALE, 0, &win, &ren);
	SDL_SetRenderVSync(ren, 1);

	palettes[0] = 0xFFFFFFFF;
	palettes[1] = 0xA9A9A9FF;
	palettes[2] = 0x545454FF;
	palettes[3] = 0x000000FF;

	tex = SDL_CreateTexture(
		ren,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		160,
		144
	);

	SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui_ImplSDL3_InitForSDLRenderer(win, ren);
	ImGui_ImplSDLRenderer3_Init(ren);
}

PPU::~PPU()
{
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyWindow(win);
	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(ren);
	SDL_Quit();
}

void PPU::load_dmg_palettes()
{
	palette_ram[0x0] = 0xFF;
	palette_ram[0x1] = 0xFF;
	palette_ram[0x2] = 0x55;
	palette_ram[0x3] = 0x55;
	palette_ram[0x4] = 0xAA;
	palette_ram[0x5] = 0xAA;
	palette_ram[0x6] = 0x00;
	palette_ram[0x7] = 0x00;

	std::copy(palette_ram.begin(), palette_ram.begin() + 7, oam_palette_ram.begin());
}

void PPU::fill_palettes()
{
	std::fill(oam_palette_ram.begin(), oam_palette_ram.end(), 0xFF);
	std::fill(palette_ram.begin(), palette_ram.end(), 0xFF);
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
		case 0xFF56: return obj_dma;
		case 0xFF47: return bgp;
		case 0xFF48: return obp0;
		case 0xFF49: return obp1;
		case 0xFF4A: return wy;
		case 0xFF4B: return wx;
		case 0xFF4C: return key0 << 2;
		case 0xFF4F: return vbk;
		case 0xFF51: return dma_src >> 8;
		case 0xFF52: return dma_src & 0xFF;
		case 0xFF53: return dma_dest >> 8;
		case 0xFF54: return dma_dest & 0xFF;
		case 0xFF55: return dma_trn;
		case 0xFF68: return bcps;
		case 0xFF69: return palette_ram[bcps & 0x3F];
		case 0xFF6A: return ocps;
		case 0xFF6B: return oam_palette_ram[ocps & 0x3F];
		case 0xFF6C: return opri;
	}
	return 0xFF;
}

void PPU::iowrite(uint16_t addr, uint8_t val)
{
	switch (addr) {
		case 0xFF40:
			if (!(val & 1)) val &= ~(1 << 5);

			if (!(val & 0x80)) {
				ly = dot = 0;
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
		case 0xFF46: obj_dma = val; obj_dma_transfer(); break;
		case 0xFF47: bgp = val; break;
		case 0xFF48: obp0 = val; break;
		case 0xFF49: obp1 = val; break;
		case 0xFF4A: wy = val; break;
		case 0xFF4B: wx = val; break;
		case 0xFF4C: key0 = (val >> 2) & 1; break;
		case 0xFF4F: vbk = val & 1; break;
		case 0xFF51: dma_src = (dma_src & 0xF0) | (val << 8); break;
		case 0xFF52: dma_src = (dma_src & 0xFF00) | (val & 0xF0); break;
		case 0xFF53: dma_dest = (dma_dest & 0xF) | (val << 8); break;
		case 0xFF54: dma_dest = (dma_dest & 0xFF00) | (val & 0xF0); break;

		case 0xFF55: {
			int length = (val & 0x7F) / 0x10 - 1;

			if (!(val & 0x80)) {
				for (int i = 0; i < length; i++) {
					uint8_t data = gb->mmu.read(dma_src + i);
					gb->mmu.write(dma_dest + i, data);
				}
			}

			dma_trn = 0xFF;
		}
			break;

		case 0xFF68: bcps = val; break;
		case 0xFF69: {
			palette_ram[bcps & 0x3F] = val;

			if (bcps & 0x80) {
				bcps &= ~(1 << 6);
				bcps++;
			}
		}
			break;

		case 0xFF6A: ocps = val; break;
		case 0xFF6B: {
			oam_palette_ram[ocps & 0x3F] = val;

			if (ocps & 0x80) {
				ocps &= ~(1 << 6);
				ocps++;
			}
		}
			break;

		case 0xFF6C: opri = val & 1; break;
	}
}

uint8_t PPU::read_vram(uint16_t addr)
{
	return vram[vbk * 0x2000 + (addr - 0x8000)];
}

void PPU::write_vram(uint16_t addr, uint8_t val)
{
	vram[vbk * 0x2000 + (addr - 0x8000)] = val;
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

uint8_t PPU::get_bgid(uint8_t bgx, uint8_t* attr)
{
	uint16_t tilemap_base;
	uint8_t x, y;

	if ((lcdc >> 5) & 1 && (wx < 167 && wy < 144) && ((bgx + 7) >= wx && ly >= wy)) {
		tilemap_base = ((lcdc >> 6) & 1) ? 0x1C00 : 0x1800;
		x = bgx - (wx - 7);
		y = wly - wy;
	}
	else {
		tilemap_base = ((lcdc >> 3) & 1) ? 0x1C00 : 0x1800;
		x = (scx + bgx) % 256;
		y = (scy + ly) % 256;
	}

	uint16_t tmap_idx = tilemap_base + ((y / 8) * 32 + (x / 8));

	uint8_t bg_attr = vram[0x2000 + tmap_idx];
	*attr = bg_attr;

	uint8_t xdir, ydir;
	ydir = (bg_attr >> 6) & 1 ? 7 - (y % 8) : (y % 8);
	xdir = (bg_attr >> 5) & 1 ? 0x1 << (x % 8) : 0x80 >> (x % 8);

	uint16_t idx = 0x2000 * ((bg_attr >> 3) & 1) + get_tile(vram[tmap_idx]) + 2 * ydir;

	uint8_t p1 = vram[idx];
	uint8_t p2 = vram[idx + 1];

	bool b1 = (p1 & xdir) != 0;
	bool b2 = (p2 & xdir) != 0;
	uint8_t id = (b2 << 1) | b1;

	return id;
}

void PPU::render_scanline()
{
	for (int i = 0; i < 160; i++) {
		uint8_t bg_attr;
		uint8_t id = get_bgid(i, &bg_attr);

		uint16_t pal_idx = 2 * (4 * (bg_attr & 0x7) + id);

		uint16_t color = (palette_ram[pal_idx + 1] << 8) | palette_ram[pal_idx];
		uint32_t rgb = to_rgb888(color);

		framebuf[idx(i, ly)] = rgb;
	}
}

void PPU::obj_dma_transfer()
{
	uint16_t src = obj_dma << 8;

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

	if (opri && key0) {
		std::sort(active_obj.begin(), active_obj.end(), [](const ObjEntry& a, const ObjEntry& b)
		{
			return a.x > b.x;
		});
	}
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
			if (i.flags & 0x40)
				tileid = (ly - y >= 8) ? i.tileid & 0xFE : i.tileid | 0x1;
			else
				tileid = (ly - y >= 8) ? i.tileid | 0x1 : i.tileid & 0xFE;
		}
		else {
			tileid = i.tileid;
		}

		uint8_t ydir = (i.flags & 0x40) ? 7 - ((ly - y) % 8) : ((ly - y) % 8);

		uint16_t tileoff = ((i.flags >> 3) & 1) * 0x2000 + tileid * 16 + 2 * ydir;

		uint8_t p1 = vram[tileoff];
		uint8_t p2 = vram[tileoff + 1];

		for (int j = 0; j < 8 && x + j < 160; j++) {
			uint8_t xdir = (i.flags & 0x20) ? 0x1 << j : 0x80 >> j;

			bool b1 = (p1 & xdir) != 0;
			bool b2 = (p2 & xdir) != 0;
			
			uint8_t obj_id = (b2 << 1) | b1;
			uint8_t bg_attr;
			uint8_t bg_id = get_bgid(x + j, &bg_attr);
			
			uint16_t pal_idx = 2 * (4 * (i.flags & 0x7) + obj_id);

			uint16_t color = (oam_palette_ram[pal_idx + 1] << 8) | oam_palette_ram[pal_idx];
			uint32_t rgb = to_rgb888(color);

			if (obj_id & 0x3) {
				if ((!(i.flags & 0x80) && !(bg_attr & 0x80)) || !bg_id || !(lcdc & 1)) {
					framebuf[idx(x + j, ly)] = rgb;
				}
			}
		}
	}
	active_obj.clear();

	if (!gb->cgb) {
		std::sort(active_obj.begin(), active_obj.end(), [](const ObjEntry& a, const ObjEntry& b)
		{
			return a.x > b.x;
		});
	}
}
 
uint16_t PPU::get_tile(uint8_t off)
{
	return (lcdc & 0x10) ? off * 16 : 0x1000 + static_cast<int8_t>(off) * 16;
}

uint32_t PPU::to_rgb888(uint16_t rgb)
{
	uint8_t r = rgb & 0x1F;
	uint8_t g = (rgb >> 5) & 0x1F;
	uint8_t b = (rgb >> 10) & 0x1F;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	return (r << 24) | (g << 16) | (b << 8) | 0xFF;
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
			if (dot == 172) {
				mode = HBLANK;
				dot = 0;

				render_scanline();
				render_sprites();
				check_stat_int();
			}
			break;

		case HBLANK:
			if (dot == 204) {
				wly += (wx < 167 && wy < 144);
				dot = 0;

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
	SDL_UpdateTexture(tex, NULL, framebuf.data(), 160 * sizeof(uint32_t));
	SDL_RenderClear(ren);

	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		ImGui::OpenPopup("popup");
	}

	if (ImGui::BeginPopup("popup")) {
		if (ImGui::MenuItem("Open ROM")) {
			gb->load_file();
		}
		ImGui::EndPopup();
	}

	ImGui::Render();
	
	SDL_RenderTexture(ren, tex, nullptr, nullptr);

	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ren);
	SDL_RenderPresent(ren);

	frame_ready = false;
}

void PPU::reset()
{
	lcdc = 0x91;
	stat = 0x84;
	lyc = ly = 0;
	scy = scx = 0;
	wx = wy = wly = 0;
	mode = OAM_SCAN;
	dot = 0;
	bgp = obp0 = obp1 = 0;
	obj_dma = 0;
	vbk = 0;
	frame_ready = false;

	std::fill(vram.begin(), vram.end(), 0);
	std::fill(framebuf.begin(), framebuf.end(), palettes[0]);
}