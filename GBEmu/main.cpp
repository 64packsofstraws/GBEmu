#include "GB.h"

int main(int argc, char** argv)
{
	GB gb;

	bool loop = gb.idle_loop();
	if (!loop) gb.run();
	return 0;
}