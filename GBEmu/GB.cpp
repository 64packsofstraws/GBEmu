#include "GB.h"

GB::GB() : cpu(this), mmu(this), timer(this), io(this), joyp(this), ppu(this)
{
	
}

int GB::idle_loop()
{
	SDL_Event e;
	int state = 1;

	while (state > 0) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_EVENT_QUIT:
				state = -1;
				break;
			}
			ImGui_ImplSDL3_ProcessEvent(&e);
		}

		SDL_SetRenderDrawColor(ppu.ren, 0, 0, 0, 255);
		SDL_RenderClear(ppu.ren);
		SDL_SetRenderTarget(ppu.ren, ppu.tex);

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("popup");
		}

		if (ImGui::BeginPopupContextItem("popup")) {
			if (ImGui::MenuItem("Open ROM")) {
				state = !load_file();
			}
			ImGui::End();
		}

		ImGui::Render();

		SDL_SetRenderTarget(ppu.ren, nullptr);
		SDL_RenderTexture(ppu.ren, ppu.tex, nullptr, nullptr);

		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), ppu.ren);
		SDL_RenderPresent(ppu.ren);
	}
	return state;
}

bool GB::load_file()
{
	nfdchar_t* out_path = NULL;
	const nfdchar_t* filter = "gb,gbc";

	nfdresult_t result = NFD_OpenDialog(filter, NULL, &out_path);

	if (result == NFD_ERROR || result == NFD_CANCEL) {
		free(out_path);
		return false;
	}

	std::ifstream f(out_path, std::ios::binary);

	uint8_t header[0x50];
	size_t rom_size, ram_size;

	f.seekg(0x100, std::ios::beg);
	f.read((char*)header, sizeof(header));

	rom_size = 0x8000 * (1 << header[0x48]);

	switch (header[0x49]) {
		case 0: ram_size = 0; break;
		case 2: ram_size = 1 << 13; break;
		case 3: ram_size = 1 << 15; break;
		case 4: ram_size = 1 << 17; break;
		case 5: ram_size = 1 << 16; break;
		default: ram_size = 0;
	}

	f.seekg(0x0, std::ios::beg);

	std::vector<uint8_t> rom(rom_size), ram(ram_size);

	f.read((char*)rom.data(), rom_size);

	uint8_t cart_type = header[0x47];
	if (cart_type == 0)
		mbc = std::make_unique<MBC0>(rom, ram);
	if (cart_type >= 1 && cart_type <= 3)
		mbc = std::make_unique<MBC1>(rom, ram);
	if (cart_type >= 0xF && cart_type <= 0x13)
		mbc = std::make_unique<MBC3>(rom, ram); 

	f.close();

	std::string title = out_path;

	title = title.substr(title.find_last_of('\\') + 1);

	SDL_SetWindowTitle(ppu.win, ("GBEmu - " + title).c_str());

	cpu.reset();
	ppu.reset();

	free(out_path);
	return true;
}
void GB::run()
{
	bool running = true;
	SDL_Event e;

	while (running) {
		Uint64 tick = SDL_GetTicks();
		
		while (!ppu.frame_ready)
			cpu.step();

		ppu.render();

		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_EVENT_QUIT:
					running = false;
					break;

				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP:
					joyp.update_joyp(e.type, e.key.key);
			}
			ImGui_ImplSDL3_ProcessEvent(&e);
		}

		Uint64 elapsed = SDL_GetTicks() - tick;

		if (elapsed > (1000 / 60))
			SDL_Delay(elapsed - (1000 / 60));
	}
}
