#include "tui.hpp"

#define TUI_BANNER "| Open-rc TUI |"



static bool tui_running;

static size_t scrw, scrh;
static size_t selection = 0;
enum {
	CUR_RUNLEVEL,
	CUR_STATUS,
	CUR_ENUM_END
};
static size_t cursor = CUR_STATUS;
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
		if(i == selection) [[ unlikely ]] {
			int cur_start, cur_len;
			switch(cursor){
				case CUR_RUNLEVEL:
					cur_start = scrw - (services[i]->status.size()+1 + services[i]->runlevel.size()+1 + 2);
					cur_len = services[i]->runlevel.size();
					break;
				case CUR_STATUS:
					cur_start = scrw - (services[i]->status.size() + 2);
					cur_len = services[i]->status.size();
					break;
			}
			// Print til cur
			wattron(wmain, A_BOLD | COLOR_PAIR(COLOR_PAIR_SELECTION));
			mvwaddnstr(wmain, lineno+1, 1, buf, cur_start);
			wattroff(wmain, COLOR_PAIR(COLOR_PAIR_SELECTION));
			// Print cur
			wattron(wmain, COLOR_PAIR(COLOR_PAIR_CURSOR));
			waddnstr(wmain, buf+cur_start, cur_len);
			wattroff(wmain, COLOR_PAIR(COLOR_PAIR_CURSOR));
			// Print rem
			wattron(wmain, COLOR_PAIR(COLOR_PAIR_SELECTION));
			waddstr(wmain, buf+cur_start+cur_len);
			wattroff(wmain, A_BOLD | COLOR_PAIR(COLOR_PAIR_SELECTION));
		}else [[ likely ]] {
			mvwaddstr(wmain, lineno+1, 1, buf);
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
		case 'h':
			if(cursor != 0){
				--cursor;
			}else{
				cursor = CUR_ENUM_END-1;
			}
			return true;
		case 'l':
			++cursor;
			if(cursor == CUR_ENUM_END){
				cursor = 0;
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
