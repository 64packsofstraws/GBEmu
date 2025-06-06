#pragma once
#include <fstream>
#include <memory>
#include <Windows.h>
#include "CPU.h"
#include "MMU.h"
#include "MBC.h"
#include "Joypad.h"
#include "IO.h"
#include "Timer.h"
#include "PPU.h"

class GB
{
public:
	std::unique_ptr<MBC> mbc;
	CPU cpu;
	PPU ppu;
	MMU mmu;
	Joypad joyp;
	Timer timer;
	IO io;

	GB();
	int idle_loop();
	bool load_file();
	void run();
};