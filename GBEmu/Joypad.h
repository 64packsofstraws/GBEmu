#pragma once
#include <cstdint>
#include <SDL3/SDL_events.h>

class GB;

class Joypad
{
	uint8_t dpad;
	uint8_t buttons;
	uint8_t sel;

	GB* gb;
public:
	Joypad(GB* gb);

	void update_joyp(Uint32 etype, SDL_Keycode k);
	void write_joyp(uint8_t val);
	uint8_t read_joyp();
};
