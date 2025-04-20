#include "MMU.h"
#include "GB.h"

MMU::MMU(GB* gb) : gb(gb)
{
}

uint8_t MMU::read(uint16_t addr)
{
    if (addr >= 0x0 && addr <= 0x7FFF) {
        return gb->mbc->cart_read(addr);
    }
    else if (addr >= 0xA000 && addr <= 0x9FFF) {
        return gb->ppu.read_vram(addr);
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        return gb->mbc->cart_read(addr);
    }
    else if (addr >= 0xC000 && addr <= 0xDFFF) {
        return gb->cpu.read_wram(addr);
    }
    else if (addr >= 0xE000 && addr <= 0xFDFF) {
        return gb->cpu.read_wram(addr - 0x2000);
    }
    else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return gb->ppu.read_oam(addr);
    }
    else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        return gb->io.io_read(addr);
    }
    else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        return gb->cpu.read_hram(addr);
    }
    else if (addr == 0xFFFF) {
        return gb->cpu.read_ie();
    }

    return 0xFF;
}

void MMU::write(uint16_t addr, uint8_t val)
{
    if (addr >= 0x0 && addr <= 0x7FFF) {
        gb->mbc->cart_write(addr, val);
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) {
        gb->ppu.write_vram(addr, val);
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        gb->mbc->cart_write(addr, val);
    }
    else if (addr >= 0xC000 && addr <= 0xDFFF) {
        gb->cpu.write_wram(addr, val);
    }
    else if (addr >= 0xE000 && addr <= 0xFDFF) {
        gb->cpu.write_wram(addr - 0x2000, val);
    }
    else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        gb->ppu.write_oam(addr, val);
    }
    else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        gb->io.io_write(addr, val);
    }
    else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        gb->cpu.write_hram(addr, val);
    }
    else if (addr == 0xFFFF) {
        gb->cpu.write_ie(val);
    }
}
