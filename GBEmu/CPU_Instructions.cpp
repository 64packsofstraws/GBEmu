#include "CPU.h"

void CPU::ld(uint16_t addr, uint8_t val)
{
	cpu_write(addr, val);
}

void CPU::ldi(uint16_t addr, uint8_t val)
{
	ld(HL, A);

	uint16_t res = HL + 1;

	H = res >> 8;
	L = res & 0xFF;
}

void CPU::ldd(uint16_t addr, uint8_t val)
{
	ld(HL, A);

	uint16_t res = HL - 1;

	H = res >> 8;
	L = res & 0xFF;
}

void CPU::ldi(uint8_t& reg, uint8_t val)
{
	ld(A, cpu_read(HL));

	uint16_t res = HL + 1;

	H = res >> 8;
	L = res & 0xFF;
}

void CPU::ldd(uint8_t& reg, uint8_t val)
{
	ld(A, cpu_read(HL));

	uint16_t res = HL - 1;

	H = res >> 8;
	L = res & 0xFF;
}

void CPU::ld(uint8_t& reg1, uint8_t& reg2, uint16_t val)
{
	reg1 = val >> 8;
	reg2 = val & 0xFF;
}

void CPU::ld(uint8_t& reg, uint8_t val)
{
	reg = val;
}

void CPU::ld_sp(uint16_t addr)
{
	cpu_write(addr, SP & 0xFF);
	cpu_write(addr + 1, SP >> 8);
}

void CPU::ld_hl_sp()
{
	int8_t off = next_byte();
	ld(H, L, SP + off);
	tick_components(1);

	Z = 0;
	N = 0;
	HC = ((SP & 0xF) + (off & 0xF)) > 0xF;
	CY = ((SP & 0xFF) + (off & 0xFF)) > 0xFF;
}

void CPU::inc_hl()
{
	uint8_t tmp = cpu_read(HL) + 1;
	cpu_write(HL, tmp);

	Z = (tmp == 0);
	N = 0;
	HC = (tmp & 0xF) == 0;
}

void CPU::inc(uint8_t& reg1, uint8_t& reg2)
{
	tick_components(1);
	uint16_t pair = ((reg1 << 8) | reg2) + 1;

	reg1 = pair >> 8;
	reg2 = pair & 0xFF;
}

void CPU::inc(uint8_t& reg)
{
	reg++;

	Z = (reg == 0);
	N = 0;
	HC = (reg & 0xF) == 0;
}

void CPU::dec_hl()
{
	uint8_t tmp = cpu_read(HL) - 1;
	cpu_write(HL, tmp);

	Z = (tmp == 0);
	N = 1;
	HC = (tmp & 0xF) == 0xF;
}

void CPU::dec(uint8_t& reg1, uint8_t& reg2)
{
	tick_components(1);
	uint16_t pair = ((reg1 << 8) | reg2) - 1;

	reg1 = pair >> 8;
	reg2 = pair & 0xFF;
}

void CPU::dec(uint8_t& reg)
{
	reg--;

	Z = (reg == 0);
	N = 1;
	HC = (reg & 0xF) == 0xF;
}

void CPU::add(uint8_t& reg1, uint8_t& reg2, uint16_t val)
{
	tick_components(1);
	uint16_t pair = (reg1 << 8) | reg2;
	uint32_t res = pair + val;

	N = 0;
	HC = ((pair & 0xFFF) + (val & 0xFFF)) > 0xFFF;
	CY = res > 0xFFFF;

	reg1 = res >> 8;
	reg2 = res & 0xFF;
}

void CPU::add(uint8_t val)
{
	uint16_t res = A + val;

	Z = ((res & 0xFF) == 0);
	N = 0;
	HC = ((A & 0xF) + (val & 0xF)) > 0xF;
	CY = res > 0xFF;

	A = res & 0xFF;
}

void CPU::add_sp_e(int8_t val)
{
	tick_components(2);
	Z = 0;
	N = 0;
	HC = ((SP & 0xF) + (val & 0xF)) > 0xF;
	CY = ((SP & 0xFF) + (val & 0xFF)) > 0xFF;

	SP += val;
}

void CPU::adc(uint8_t val)
{
	uint16_t res = A + val + CY;

	Z = ((res & 0xFF) == 0);
	N = 0;
	HC = ((A & 0xf) + (val & 0xf) + CY) > 0xF;
	CY = res > 0xFF;

	A = res & 0xFF;
}

void CPU::sbc(uint8_t val)
{
	int16_t res = A - val - CY;

	Z = ((res & 0xFF) == 0);
	N = 1;
	HC = ((A & 0xF) - (val & 0xF) - CY) & 0x10;
	CY = res < 0;

	A = res & 0xFF;
}

void CPU::anda(uint8_t val)
{
	A &= val;

	Z = (A == 0);
	N = 0;
	HC = 1;
	CY = 0;
}

void CPU::xora(uint8_t val)
{
	A ^= val;

	Z = (A == 0);
	N = 0;
	HC = 0;
	CY = 0;
}

void CPU::ora(uint8_t val)
{
	A |= val;

	Z = (A == 0);
	N = 0;
	HC = 0;
	CY = 0;
}

void CPU::cp(uint8_t val)
{
	int16_t res = A - val;

	Z = ((res & 0xFF) == 0);
	N = 1;
	HC = ((A & 0xF) - (val & 0xF)) & 0x10;
	CY = res < 0;
}

void CPU::sub(uint8_t val)
{
	int16_t res = A - val;

	Z = ((res & 0xFF) == 0);
	N = 1;
	HC = ((A & 0xF) - (val & 0xF)) & 0x10;
	CY = res < 0;

	A = res & 0xFF;
}

void CPU::rlc(uint8_t& reg)
{
	CY = (reg >> 7);
	N = 0;
	HC = 0;

	reg <<= 1;
	reg |= CY;
	Z = (reg == 0);
}

void CPU::rlca()
{
	CY = (A >> 7);
	Z = 0;
	N = 0;
	HC = 0;

	A <<= 1;
	A |= CY;
}

void CPU::rl(uint8_t& reg)
{
	bool prev_cy = CY;
	CY = (reg >> 7);
	N = 0;
	HC = 0;

	reg <<= 1;
	reg |= prev_cy;

	Z = (reg == 0);
}

void CPU::rla()
{
	bool prev_cy = CY;
	CY = (A >> 7);
	N = 0;
	HC = 0;
	Z = 0;

	A <<= 1;
	A |= prev_cy;
}

void CPU::rrc(uint8_t& reg)
{
	CY = reg & 1;
	reg >>= 1;
	reg |= (CY << 7);

	Z = (reg == 0);
	N = 0;
	HC = 0;
}

void CPU::rrca()
{
	CY = A & 1;
	A >>= 1;
	A |= (CY << 7);

	Z = 0;
	N = 0;
	HC = 0;
}

void CPU::rr(uint8_t& reg)
{
	bool prev_cy = CY;
	CY = reg & 1;
	reg >>= 1;

	reg |= (prev_cy << 7);
	Z = (reg == 0);
	N = 0;
	HC = 0;
}

void CPU::rra()
{
	bool prev_cy = CY;
	CY = A & 1;
	A >>= 1;

	A |= (prev_cy << 7);
	Z = 0;
	N = 0;
	HC = 0;
}

void CPU::sla(uint8_t& reg)
{
	CY = (reg >> 7);

	reg <<= 1;

	Z = (reg == 0);
	N = 0;
	HC = 0;
}

void CPU::sra(uint8_t& reg)
{
	CY = (reg & 1);
	uint8_t b = reg & 0x80;

	reg >>= 1;
	reg |= b;

	Z = (reg == 0);
	N = 0;
	HC = 0;
}

void CPU::srl(uint8_t& reg)
{
	CY = (reg & 1);

	reg >>= 1;

	Z = (reg == 0);
	N = 0;
	HC = 0;
}

void CPU::swap(uint8_t& reg)
{
	uint8_t hn = reg >> 4;
	uint8_t ln = reg & 0xF;

	reg = (ln << 4) | hn;

	Z = (reg == 0);
	N = 0;
	HC = 0;
	CY = 0;
}

void CPU::bit(uint8_t b, uint8_t val)
{
	Z = (val & (1 << b)) == 0;
	N = 0;
	HC = 1;
}

void CPU::res(uint8_t b, uint8_t& reg)
{
	reg &= ~(1 << b);
}

void CPU::set(uint8_t b, uint8_t& reg)
{
	reg |= (1 << b);
}

void CPU::jp(uint16_t addr)
{
	tick_components(1);
	PC = addr;
}

void CPU::jp_cond(bool cond, uint16_t addr)
{
	if (cond) {
		tick_components(1);
		PC = addr;
	}
}

void CPU::jr(uint8_t off)
{
	tick_components(1);
	PC += static_cast<int8_t>(off);
}

void CPU::jr_cond(bool cond, uint8_t off)
{
	if (cond) {
		tick_components(1);
		PC += static_cast<int8_t>(off);
	}
}

void CPU::call(uint16_t addr)
{
	push(PC);
	PC = addr;
}

void CPU::call_cond(bool cond, uint16_t addr)
{
	if (cond) call(addr);
}

void CPU::ret()
{
	uint8_t h = cpu_read(SP + 1);
	uint8_t l = cpu_read(SP);
	SP += 2;

	PC = (h << 8) | l;
	tick_components(1);
}

void CPU::ret_cond(bool cond)
{
	tick_components(1);
	if (cond) ret();
}

void CPU::push(uint16_t val)
{
	cpu_write(SP - 1, val >> 8);
	cpu_write(SP - 2, val & 0xFF);
	SP -= 2;
	tick_components(1);
}

void CPU::push_af()
{
	uint8_t F = 0;

	F |= (CY << 4);
	F |= (HC << 5);
	F |= (N << 6);
	F |= (Z << 7);

	push((A << 8) | F);
}

void CPU::pop(uint8_t& reg1, uint8_t& reg2)
{
	reg1 = cpu_read(SP + 1);
	reg2 = cpu_read(SP);

	SP += 2;
}

void CPU::pop_af()
{
	uint8_t F = 0;

	pop(A, F);

	CY = (F & (1 << 4)) != 0;
	HC = (F & (1 << 5)) != 0;
	N = (F & (1 << 6)) != 0;
	Z = (F & (1 << 7)) != 0;
}

void CPU::daa()
{
	uint8_t correct = 0;

	if (HC || (!N && (A & 0xF) > 9)) correct |= 0x6;

	if (CY || (!N && A > 0x99)) {
		correct |= 0x60;
		CY = 1;
	}

	A += N ? -correct : correct;

	A &= 0xFF;

	Z = (A == 0);
	HC = 0;
}

void CPU::cpl()
{
	N = 1;
	HC = 1;
	A = ~A;
}

void CPU::ccf()
{
	N = 0;
	HC = 0;
	CY = !CY;
}

void CPU::scf()
{
	N = 0;
	HC = 0;
	CY = 1;
}

void CPU::halt()
{
	if ((ie & intf) == 0) PC--;
}

uint8_t* CPU::get_register(uint8_t n)
{
	switch (n % 0x8) {
	case 0: return &B;
	case 1: return &C;
	case 2: return &D;
	case 3: return &E;
	case 4: return &H;
	case 5: return &L;
	case 7: return &A;
	}

	return nullptr;
}

uint8_t CPU::prefix_cb()
{
	uint8_t op = cpu_read(PC++);
	uint8_t* reg = get_register(op & 0x7);
	uint8_t cycles = 1;

	if (op < 0x40) {
		if (reg) {
			switch (op >> 3) {
			case 0x00: rlc(*reg); break;
			case 0x01: rrc(*reg); break;
			case 0x02: rl(*reg); break;
			case 0x03: rr(*reg); break;
			case 0x04: sla(*reg); break;
			case 0x05: sra(*reg); break;
			case 0x06: swap(*reg); break;
			case 0x07: srl(*reg); break;
			}
		} 
		else {
			uint8_t tmp = cpu_read(HL);

			switch (op >> 3) {
			case 0x00: rlc(tmp); break;
			case 0x01: rrc(tmp); break;
			case 0x02: rl(tmp); break;
			case 0x03: rr(tmp); break;
			case 0x04: sla(tmp); break;
			case 0x05: sra(tmp); break;
			case 0x06: swap(tmp); break;
			case 0x07: srl(tmp); break;
			}

			cpu_write(HL, tmp);
		}
	}
	else {
		uint8_t idx = (op >> 3) & 0x7;

		if (reg) {
			switch (op >> 6) {
			case 0x1: bit(idx, *reg); break;
			case 0x2: res(idx, *reg); break;
			case 0x3: set(idx, *reg); break;
			}
		}
		else {
			uint8_t tmp = cpu_read(HL);

			switch (op >> 6) {
			case 0x1: bit(idx, tmp); break;
			case 0x2: res(idx, tmp); cpu_write(HL, tmp); break;
			case 0x3: set(idx, tmp); cpu_write(HL, tmp); break;
			}
		}
	}

	return cycles;
}