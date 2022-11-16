#include "tui.hpp"

#define TUI_BANNER "| Open-rc TUI |"



static bool tui_running;

static size_t scrw, scrh;
static size_t selection = 0;
static size_t cursor = 0;
static size_t state = 0;

inline WINDOW* wmain;
inline WINDOW* whelpbar;

enum {
	COLOR_PAIR_SELECTION = 1,
	COLOR_PAIR_CURSOR,
	COLOR_PAIR_HELP
};

bool tui_init(){
	initscr();

	noecho();
	curs_set(0);

	getmaxyx(stdscr, scrh, scrw);

	start_color();
	init_pair(COLOR_PAIR_SELECTION, COLOR_WHITE, COLOR_RED);
	init_pair(COLOR_PAIR_CURSOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(COLOR_PAIR_HELP, COLOR_BLACK, COLOR_WHITE);

	wmain = newwin(scrh-1, scrw, 0, 0);
	whelpbar = newwin(1, scrw, scrh-1, 0);

	refresh();

	tui_running = true;
	return true;
}

void tui_quit(){
	if(not tui_running){ return; }
	endwin();
}

void tui_services(){
	int lineno = 0;
	for(int i = 0; i < services.size(); i++){
		char* buf = services[i]->pretty_render(scrw-2);
		if(i == selection){
			wattron(wmain, A_BOLD | COLOR_PAIR(COLOR_PAIR_SELECTION));
		}
		mvwaddstr(wmain, lineno+1, 1, buf);
		if(i == selection){
			wattroff(wmain, A_BOLD | COLOR_PAIR(COLOR_PAIR_SELECTION));
		}
		++lineno;
		delete buf;
	}
}

static const char** HELP_MSG[] = {
							(char const *[]){"Down [j]", "Up [k]", "Next [h]", "Previous [l]", "Modify [\\n]", NULL}
						};

void tui_help(){
	werase(whelpbar);
	for(int i = 0; HELP_MSG[state][i] != NULL; i++){
		waddch(whelpbar, ' ');
		wattron(whelpbar, COLOR_PAIR(COLOR_PAIR_HELP));
		waddstr(whelpbar, HELP_MSG[state][i]);
		wattroff(whelpbar, COLOR_PAIR(COLOR_PAIR_HELP));
	}
}

bool tui_control(const char &c){
	switch(c){
		case 'j':
			if(selection < services.size()-1){
				++selection;
			}else{
				selection = 0;
			}
			return true;
		case 'k':
			if(selection > 0){
				--selection;
			}else{
				selection = services.size()-1;
			}
			return true;
	}

	return false;
}

void tui_display(){
	box(wmain, 0, 0);
	mvwaddstr(wmain, 0, (scrw-(sizeof(TUI_BANNER)-1))/2, TUI_BANNER);
	tui_services();
	tui_help();
	//
	wrefresh(wmain);
	wrefresh(whelpbar);
}
