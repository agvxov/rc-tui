#include "tui.hpp"

static bool tui_running;

static size_t scrw, scrh;

static WINDOW* wmain;
static WINDOW* whelpbar;

bool tui_init(){
	initscr();

	getmaxyx(stdscr, scrh, scrw);

	wmain = newwin(scrh-1, scrw, 0, 0);
	whelpbar = newwin(1, scrw, scrh-1, 0);

	refresh();

	//
	box(wmain, '|', '-');
	wrefresh(wmain);
	waddstr(whelpbar, "test");
	wrefresh(whelpbar);
	//
	refresh();

	tui_running = true;
	return true;
}

void tui_quit(){
	if(not tui_running){ return; }
	endwin();
}
