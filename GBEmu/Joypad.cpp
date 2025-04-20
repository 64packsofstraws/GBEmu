#include "Joypad.h"
#include "GB.h"

Joypad::Joypad(GB* gb) : gb(gb)
{
    buttons = dpad = 0xF;
    sel = 0x30;
}

void Joypad::update_joyp(Uint32 etype, SDL_Keycode k)
{
    if (etype == SDL_EVENT_KEY_DOWN) {
        switch (k) {
        case SDLK_Z: buttons &= ~(0x1); break;
        case SDLK_X: buttons &= ~(0x2); break;
        case SDLK_SPACE: buttons &= ~(0x4); break;
        case SDLK_RETURN: buttons &= ~(0x8); break;

        case SDLK_RIGHT: dpad &= ~(0x1); break;
        case SDLK_LEFT: dpad &= ~(0x2); break;
        case SDLK_UP: dpad &= ~(0x4); break;
        case SDLK_DOWN: dpad &= ~(0x8); break;
        }

        gb->cpu.req_intf(4);
    }
    else if (etype == SDL_EVENT_KEY_UP) {
        switch (k) {
        case SDLK_Z: buttons |= 0x1; break;
        case SDLK_X: buttons |= 0x2; break;
        case SDLK_SPACE: buttons |= 0x4; break;
        case SDLK_RETURN: buttons |= 0x8; break;

        case SDLK_RIGHT: dpad |= 0x1; break;
        case SDLK_LEFT: dpad |= 0x2; break;
        case SDLK_UP: dpad |= 0x4; break;
        case SDLK_DOWN: dpad |= 0x8; break;
        }
    }
}

void Joypad::write_joyp(uint8_t val)
{
    sel = val & 0x30;
}

uint8_t Joypad::read_joyp()
{
    if (!(sel & 0b00100000)) return 0xF0 | buttons;
    if (!(sel & 0b00010000)) return 0xF0 | dpad;

    return 0xFF;
}
