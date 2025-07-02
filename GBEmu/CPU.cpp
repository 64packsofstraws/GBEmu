#include "CPU.h"
#include "GB.h"

CPU::CPU(GB* gb) : wram(0x8000, 0), hram(0x80, 0), gb(gb)
{
	reset();
}

void CPU::tick_components(uint8_t m)
{
	for (int i = 0; i < m; i++) gb->timer.tick();
	for (int i = 0; i < m * 4; i++) gb->ppu.tick();
}

uint8_t CPU::read_wram(uint16_t addr)
{
	if (addr < 0xD000 || !gb->cgb) return wram[addr - 0xC000];
	else return wram[(!svbk ? 1 : svbk) * 0x1000 + (addr - 0xD000)];
}

void CPU::write_wram(uint16_t addr, uint8_t val)
{
	if (addr < 0xD000 || !gb->cgb) wram[addr - 0xC000] = val;
	else wram[(!svbk ? 1 : svbk) * 0x1000 + (addr - 0xD000)] = val;
}

uint8_t CPU::read_hram(uint16_t addr)
{
	return hram[addr - 0xFF80];
}

void CPU::write_hram(uint16_t addr, uint8_t val)
{
	hram[addr - 0xFF80] = val;
}

uint8_t CPU::read_intf()
{
	return intf;
}

void CPU::write_intf(uint8_t val)
{
	intf = val;
}

void CPU::req_intf(uint8_t bit)
{
	intf |= (1 << bit);
}

uint8_t CPU::read_ie()
{
	return ie;
}

void CPU::write_ie(uint8_t val)
{
	ie = val;
}

uint8_t CPU::read_svbk()
{
	return svbk;
}

void CPU::write_svbk(uint8_t val)
{
	svbk = val & 0x7;
}

void CPU::reset()
{
	A = 0x11;
	B = C = D = E = H = L = 0;
	PC = 0x100;
	SP = 0xFFFE;
	ime = false;
	intf = ie = 0x0;

	std::fill(wram.begin(), wram.end(), 0);
}

uint8_t CPU::handle_int()
{
	if (ime) {
		for (int i = 0; i < 5; i++) {
			uint8_t bit_sel = (1 << i);

			if ((intf & bit_sel) && (ie & bit_sel)) {
				intf &= ~(bit_sel);
				ime = false;

				call(0x40 + (0x8 * i));
				return 5;
			}
		}
	}
	return 0;
}

uint8_t CPU::cpu_read(uint16_t addr)
{
	tick_components(1);
	return gb->mmu.read(addr);
}

void CPU::cpu_write(uint16_t addr, uint8_t val)
{
	tick_components(1);
	gb->mmu.write(addr, val);
}

uint8_t CPU::next_byte()
{
	uint8_t val = cpu_read(PC++);
	return val;
}

uint16_t CPU::next_word()
{
	uint16_t val = (cpu_read(PC + 1) << 8) | (cpu_read(PC));
	PC += 2;
	return val;
}

void CPU::step()
{
	uint8_t op;

	uint8_t cycles = 0;
	tick_components(handle_int());

	op = cpu_read(PC++);

	switch (op) {
		case 0x0: break;
		case 0x1: ld(B, C, next_word()); break;
		case 0x2: ld(BC, A); break;
		case 0x3: inc(B, C); break;
		case 0x4: inc(B); break;
		case 0x5: dec(B); break;
		case 0x6: ld(B, next_byte()); break;
		case 0x7: rlca(); break;
		case 0x8: ld_sp(next_word()); break;
		case 0x9: add(H, L, BC); break;
		case 0xA: ld(A, cpu_read(BC)); break;
		case 0xB: dec(B, C); break;
		case 0xC: inc(C); break;
		case 0xD: dec(C); break;
		case 0xE: ld(C, next_byte()); break;
		case 0xF: rrca(); break;

		case 0x10: PC++; tick_components(1); break;
		case 0x11: ld(D, E, next_word()); break;
		case 0x12: ld(DE, A); break;
		case 0x13: inc(D, E); break;
		case 0x14: inc(D); break;
		case 0x15: dec(D); break;
		case 0x16: ld(D, next_byte()); break;
		case 0x17: rla(); break;
		case 0x18: jr(next_byte()); break;
		case 0x19: add(H, L, DE); break;
		case 0x1A: ld(A, cpu_read(DE)); break;
		case 0x1B: dec(D, E); break;
		case 0x1C: inc(E); break;
		case 0x1D: dec(E); break;
		case 0x1E: ld(E, next_byte()); break;
		case 0x1F: rra(); break;

		case 0x20: jr_cond(!Z, next_byte()); break;
		case 0x21: ld(H, L, next_word()); break;
		case 0x22: ldi(HL, A); break;
		case 0x23: inc(H, L); break;
		case 0x24: inc(H); break;
		case 0x25: dec(H); break;
		case 0x26: ld(H, next_byte()); break;
		case 0x27: daa(); break;
		case 0x28: jr_cond(Z, next_byte()); break;
		case 0x29: add(H, L, HL); break;
		case 0x2A: ldi(A, HL); break;
		case 0x2B: dec(H, L); break;
		case 0x2C: inc(L); break;
		case 0x2D: dec(L); break;
		case 0x2E: ld(L, next_byte()); break;
		case 0x2F: cpl(); break;

		case 0x30: jr_cond(!CY, next_byte()); break;
		case 0x31: SP = next_word(); break;
		case 0x32: ldd(HL, A); break;
		case 0x33: SP++; tick_components(1); break;
		case 0x34: inc_hl(); break;
		case 0x35: dec_hl(); break;
		case 0x36: ld(HL, next_byte()); break;
		case 0x37: scf(); break;
		case 0x38: jr_cond(CY, next_byte()); break;
		case 0x39: add(H, L, SP); break;
		case 0x3A: ldd(A, HL); break;
		case 0x3B: SP--; tick_components(1); break;
		case 0x3C: inc(A); break;
		case 0x3D: dec(A); break;
		case 0x3E: ld(A, next_byte()); break;
		case 0x3F: ccf(); break;

		case 0x40: ld(B, B); break;
		case 0x41: ld(B, C); break;
		case 0x42: ld(B, D); break;
		case 0x43: ld(B, E); break;
		case 0x44: ld(B, H); break;
		case 0x45: ld(B, L); break;
		case 0x46: ld(B, cpu_read(HL)); break;
		case 0x47: ld(B, A); break;
		case 0x48: ld(C, B); break;
		case 0x49: ld(C, C); break;
		case 0x4A: ld(C, D); break;
		case 0x4B: ld(C, E); break;
		case 0x4C: ld(C, H); break;
		case 0x4D: ld(C, L); break;
		case 0x4E: ld(C, cpu_read(HL)); break;
		case 0x4F: ld(C, A); break;

		case 0x50: ld(D, B); break;
		case 0x51: ld(D, C); break;
		case 0x52: ld(D, D); break;
		case 0x53: ld(D, E); break;
		case 0x54: ld(D, H); break;
		case 0x55: ld(D, L); break;
		case 0x56: ld(D, cpu_read(HL)); break;
		case 0x57: ld(D, A); break;
		case 0x58: ld(E, B); break;
		case 0x59: ld(E, C); break;
		case 0x5A: ld(E, D); break;
		case 0x5B: ld(E, E); break;
		case 0x5C: ld(E, H); break;
		case 0x5D: ld(E, L); break;
		case 0x5E: ld(E, cpu_read(HL)); break;
		case 0x5F: ld(E, A); break;

		case 0x60: ld(H, B); break;
		case 0x61: ld(H, C); break;
		case 0x62: ld(H, D); break;
		case 0x63: ld(H, E); break;
		case 0x64: ld(H, H); break;
		case 0x65: ld(H, L); break;
		case 0x66: ld(H, cpu_read(HL)); break;
		case 0x67: ld(H, A); break;
		case 0x68: ld(L, B); break;
		case 0x69: ld(L, C); break;
		case 0x6A: ld(L, D); break;
		case 0x6B: ld(L, E); break;
		case 0x6C: ld(L, H); break;
		case 0x6D: ld(L, L); break;
		case 0x6E: ld(L, cpu_read(HL)); break;
		case 0x6F: ld(L, A); break;

		case 0x70: ld(HL, B); break;
		case 0x71: ld(HL, C); break;
		case 0x72: ld(HL, D); break;
		case 0x73: ld(HL, E); break;
		case 0x74: ld(HL, H); break;
		case 0x75: ld(HL, L); break;
		case 0x76: halt(); break;
		case 0x77: ld(HL, A); break;
		case 0x78: ld(A, B); break;
		case 0x79: ld(A, C); break;
		case 0x7A: ld(A, D); break;
		case 0x7B: ld(A, E); break;
		case 0x7C: ld(A, H); break;
		case 0x7D: ld(A, L); break;
		case 0x7E: ld(A, cpu_read(HL)); break;
		case 0x7F: ld(A, A); break;

		case 0x80: add(B); break;
		case 0x81: add(C); break;
		case 0x82: add(D); break;
		case 0x83: add(E); break;
		case 0x84: add(H); break;
		case 0x85: add(L); break;
		case 0x86: add(cpu_read(HL)); break;
		case 0x87: add(A); break;
		case 0x88: adc(B); break;
		case 0x89: adc(C); break;
		case 0x8A: adc(D); break;
		case 0x8B: adc(E); break;
		case 0x8C: adc(H); break;
		case 0x8D: adc(L); break;
		case 0x8E: adc(cpu_read(HL)); break;
		case 0x8F: adc(A); break;

		case 0x90: sub(B); break;
		case 0x91: sub(C); break;
		case 0x92: sub(D); break;
		case 0x93: sub(E); break;
		case 0x94: sub(H); break;
		case 0x95: sub(L); break;
		case 0x96: sub(cpu_read(HL)); break;
		case 0x97: sub(A); break;
		case 0x98: sbc(B); break;
		case 0x99: sbc(C); break;
		case 0x9A: sbc(D); break;
		case 0x9B: sbc(E); break;
		case 0x9C: sbc(H); break;
		case 0x9D: sbc(L); break;
		case 0x9E: sbc(cpu_read(HL)); break;
		case 0x9F: sbc(A); break;

		case 0xA0: anda(B); break;
		case 0xA1: anda(C); break;
		case 0xA2: anda(D); break;
		case 0xA3: anda(E); break;
		case 0xA4: anda(H); break;
		case 0xA5: anda(L); break;
		case 0xA6: anda(cpu_read(HL)); break;
		case 0xA7: anda(A); break;
		case 0xA8: xora(B); break;
		case 0xA9: xora(C); break;
		case 0xAA: xora(D); break;
		case 0xAB: xora(E); break;
		case 0xAC: xora(H); break;
		case 0xAD: xora(L); break;
		case 0xAE: xora(cpu_read(HL)); break;
		case 0xAF: xora(A); break;

		case 0xB0: ora(B); break;
		case 0xB1: ora(C); break;
		case 0xB2: ora(D); break;
		case 0xB3: ora(E); break;
		case 0xB4: ora(H); break;
		case 0xB5: ora(L); break;
		case 0xB6: ora(cpu_read(HL)); break;
		case 0xB7: ora(A); break;
		case 0xB8: cp(B); break;
		case 0xB9: cp(C); break;
		case 0xBA: cp(D); break;
		case 0xBB: cp(E); break;
		case 0xBC: cp(H); break;
		case 0xBD: cp(L); break;
		case 0xBE: cp(cpu_read(HL)); break;
		case 0xBF: cp(A); break;

		case 0xC0: ret_cond(!Z); break;
		case 0xC1: pop(B, C); break;
		case 0xC2: jp_cond(!Z, next_word()); break;
		case 0xC3: jp(next_word()); break;
		case 0xC4: call_cond(!Z, next_word()); break;
		case 0xC5: push(BC); break;
		case 0xC6: add(next_byte()); break;
		case 0xC7: call(0x0); break;
		case 0xC8: ret_cond(Z); break;
		case 0xC9: ret(); break;
		case 0xCA: jp_cond(Z, next_word()); break;
		case 0xCB: prefix_cb(); break;
		case 0xCC: call_cond(Z, next_word()); break;
		case 0xCD: call(next_word()); break;
		case 0xCE: adc(next_byte()); break;
		case 0xCF: call(0x8); break;

		case 0xD0: ret_cond(!CY); break;
		case 0xD1: pop(D, E); break;
		case 0xD2: jp_cond(!CY, next_word()); break;
		case 0xD3: break;
		case 0xD4: call_cond(!CY, next_word()); break;
		case 0xD5: push(DE); break;
		case 0xD6: sub(next_byte()); break;
		case 0xD7: call(0x10); break;
		case 0xD8: ret_cond(CY); break;
		case 0xD9: ime = true; ret(); break;
		case 0xDA: jp_cond(CY, next_word()); break;
		case 0xDB: break;
		case 0xDC: call_cond(CY, next_word()); break;
		case 0xDD: break;
		case 0xDE: sbc(next_byte()); break;
		case 0xDF: call(0x18); break;

		case 0xE0: ld(0xFF00 + next_byte(), A); break;
		case 0xE1: pop(H, L); break;
		case 0xE2: ld(0xFF00 + C, A); break;
		case 0xE3: break;
		case 0xE4: break;
		case 0xE5: push(HL); break;
		case 0xE6: anda(next_byte()); break;
		case 0xE7: call(0x20); break;
		case 0xE8: add_sp_e(next_byte()); break;
		case 0xE9: PC = HL; break;
		case 0xEA: ld(next_word(), A); break;
		case 0xEB: break;
		case 0xEC: break;
		case 0xED: break;
		case 0xEE: xora(next_byte()); break;
		case 0xEF: call(0x28); break;

		case 0xF0: ld(A, cpu_read(0xFF00 + next_byte())); break;
		case 0xF1: pop_af(); break;
		case 0xF2: ld(A, cpu_read(0xFF00 + C)); break;
		case 0xF3: ime = false; break;
		case 0xF4: break;
		case 0xF5: push_af(); break;
		case 0xF6: ora(next_byte()); break;
		case 0xF7: call(0x30); break;
		case 0xF8: ld_hl_sp();  break;
		case 0xF9: SP = HL; tick_components(1); break;
		case 0xFA: ld(A, cpu_read(next_word())); break;
		case 0xFB: ime = true; break;
		case 0xFC: break;
		case 0xFD: break;
		case 0xFE: cp(next_byte()); break;
		case 0xFF: call(0x38); break;
	}
}
