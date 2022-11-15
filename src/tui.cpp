#include "tui.hpp"

static tui_running;

bool tui_init(){
	initsrc();

	tui_running = true;
	return true;
}

void tui_quit(){
	if(not tui_running){ return; }
	endwin();
}
