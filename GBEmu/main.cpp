#include "GB.h"

int main(int argc, char** argv)
{
	GB gb;

	int state = gb.idle_loop();
	if (state > -1) gb.run();
	return 0;
}