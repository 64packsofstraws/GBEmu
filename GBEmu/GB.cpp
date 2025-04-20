#include "GB.h"

GB::GB(const char* filename) : cpu(this), mmu(this), timer(this), io(this), joyp(this), ppu(this)
{
	std::ifstream f(filename, std::ios::binary);

	if (!f) {
		MessageBoxA(NULL, "ROM doesn't exist", "Error", MB_ICONERROR);
		exit(1);
	}

	uint8_t header[0x50];
	size_t rom_size, ram_size;

	f.seekg(0x100, std::ios::beg);
	f.read((char*)header, sizeof(header));

	rom_size = 0x8000 * (1 << header[0x48]);

	switch (header[0x49]) {
		case 0: ram_size = 0; break;
		case 2: ram_size = 8192; break;
		case 3: ram_size = 32768; break;
		default: ram_size = 0;
	}

	f.seekg(0x0, std::ios::beg);

	std::vector<uint8_t> rom(rom_size), ram(ram_size);

	f.read((char*)rom.data(), rom_size);

	switch (header[0x47]) {
		case 0: mbc = std::make_unique<MBC0>(rom, ram); break;
		case 1: mbc = std::make_unique<MBC1>(rom, ram); break;
	}

	f.close();
}

void GB::run()
{
	bool running = true;
	SDL_Event e;
	Uint64 tick = SDL_GetTicks();

	while (running) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;

				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP:
					joyp.update_joyp(e.type, e.key.key);
			}
		}

		cpu.step();

		if (ppu.frame_ready) {
			ppu.render();

			Uint64 elapsed = SDL_GetTicks() - tick;

			if (elapsed < 24) {
				SDL_Delay(24 - elapsed);

				tick = SDL_GetTicks();
			}
		}
	}
}
