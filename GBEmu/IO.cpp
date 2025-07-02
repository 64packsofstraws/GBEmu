#include "IO.h"
#include "GB.h"

IO::IO(GB* gb) : gb(gb)
{

}

uint8_t IO::io_read(uint16_t addr)
{
    if (addr == 0xFF00) {
        return gb->joyp.read_joyp();
    }
    else if (addr >= 0xFF04 && addr <= 0xFF07) {
        return gb->timer.read_timer(addr);
    }
    else if (addr >= 0xFF40 && addr <= 0xFF6B) {
        return gb->ppu.ioread(addr);
    }
    else if (addr == 0xFF70) {
        return gb->cpu.read_svbk();
    }
    else if (addr == 0xFF0F) {
        return gb->cpu.read_intf();
    }

    return 0xFF;
}

void IO::io_write(uint16_t addr, uint8_t val)
{
    if (addr == 0xFF00) {
        gb->joyp.write_joyp(val);
    }
    else if (addr >= 0xFF04 && addr <= 0xFF07) {
        gb->timer.write_timer(addr, val);
    }
    else if (addr == 0xFF0F) {
        gb->cpu.write_intf(val);
    }
    else if (addr >= 0xFF40 && addr <= 0xFF6B) {
        gb->ppu.iowrite(addr, val);
    }
    else if (addr == 0xFF70) {
        gb->cpu.write_svbk(val);
    }
}