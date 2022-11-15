#include "tui.hpp"

#define TUI_BANNER "| Open-rc TUI |"

static void tui_services();



static bool tui_running;

static size_t scrw, scrh;
static size_t selection = 0;

static WINDOW* wmain;
static WINDOW* whelpbar;



bool tui_init(){
	initscr();

	noecho();
	curs_set(0);

	getmaxyx(stdscr, scrh, scrw);

	wmain = newwin(scrh-1, scrw, 0, 0);
	whelpbar = newwin(1, scrw, scrh-1, 0);

	refresh();

	//
	box(wmain, '|', '-');
	mvwaddstr(wmain, 0, (scrw-(sizeof(TUI_BANNER)-1))/2, TUI_BANNER);
	wrefresh(wmain);
	waddstr(whelpbar, "test");
	wrefresh(whelpbar);
	//
	//
	tui_services();
	wrefresh(wmain);

	tui_running = true;
	return true;
}

void tui_quit(){
	if(not tui_running){ return; }
	endwin();
}

static void tui_services(){
	int lineno = 0;
	for(int i = 0; i < services.size(); i++){
		char* buf = services[i]->pretty_render(scrw-2);
		if(i == selection){
			wattron(wmain, A_REVERSE | A_BOLD);
		}
		mvwaddstr(wmain, lineno+1, 1, buf);
		if(i == selection){
			wattroff(wmain, A_REVERSE | A_BOLD);
		}
		++lineno;
		delete buf;
	}
}
