#include "tui.hpp"


#define TUI_BANNER "| Open-rc TUI |"

static bool tui_running;

static size_t scrw, scrh;
enum {
	CUR_RUNLEVEL,
	CUR_STATUS,
	CUR_ENUM_END
};
static size_t cursor = CUR_STATUS;
static size_t selection = 0;

enum {
	STATE_INITIAL,
	STATE_CMD_MENU
};
static size_t state = STATE_INITIAL;

static WINDOW* wmain;
static WINDOW* wmaind;
static WINDOW* wmenu;
static WINDOW* wmenud;
static WINDOW* whelpbar;
static MENU* cmd_menu;

enum {
	COLOR_PAIR_SELECTION = 1,
	COLOR_PAIR_CURSOR,
	COLOR_PAIR_HELP
};


static inline int windoww(WINDOW* w){
	return (w->_maxx+1 - w->_begx)+1;
}
static inline int windowh(WINDOW* w){
	return (w->_maxy+1 - w->_begy+w->_yoffset)+1;
}

bool tui_init(){
	initscr();

	nonl();
	noecho();
	curs_set(0);

	getmaxyx(stdscr, scrh, scrw);

	start_color();
	init_pair(COLOR_PAIR_SELECTION, COLOR_WHITE, COLOR_RED);
	init_pair(COLOR_PAIR_CURSOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(COLOR_PAIR_HELP, COLOR_BLACK, COLOR_WHITE);

	wmain = newwin(scrh-1, scrw, 0, 0);
	wmaind = derwin(wmain, wmain->_maxy-1, wmain->_maxx-1, 1, 1);
	whelpbar = newwin(1, scrw, scrh-1, 0);

	refresh();

	tui_running = true;
	return true;
}

void tui_quit(){
	if(not tui_running){ return; }
	endwin();
}

static char* tui_render_service(const Service* const s, const size_t &width){
	char* r;

	asprintf(
				&r,
				"%s%*s%*s%*s",
				s->name.c_str(),
				(int)(width-(s->name.size()+MAX_RUNLEVEL_LEN+MAX_STATUS_LEN+1)),
				" ",
				MAX_RUNLEVEL_LEN,
				s->runlevel.c_str(),
				MAX_STATUS_LEN+1,
				s->status.c_str()
			);

	return r;
}

static size_t tui_rendered_service_button_pos(const Service* const s, size_t width){
	size_t starts[] = {
			width-(s->status.size()+1 + s->runlevel.size()+1),
			width - (s->status.size())
		};
	return starts[cursor];
}

static void tui_render_services(){
	wmove(wmaind, 0, 0);
	int winw = windoww(wmaind);
	for(int i = 0; i < services.size(); i++){
		char* buf = tui_render_service(services[i], winw);
		if(i == selection) [[ unlikely ]] {
			size_t cur_start,
				cur_len;
			cur_start = tui_rendered_service_button_pos(services[i], winw);
			switch(cursor){
				case CUR_RUNLEVEL:
					cur_len = services[i]->runlevel.size();
					break;
				case CUR_STATUS:
					cur_len = services[i]->status.size();
					break;
			}
			// Print til cur
			wattron(wmaind, A_BOLD | COLOR_PAIR(COLOR_PAIR_SELECTION));
			waddnstr(wmaind, buf, cur_start);
			wattroff(wmaind, COLOR_PAIR(COLOR_PAIR_SELECTION));
			// Print cur
			wattron(wmaind, COLOR_PAIR(COLOR_PAIR_CURSOR));
			waddnstr(wmaind, buf+cur_start, cur_len);
			wattroff(wmaind, COLOR_PAIR(COLOR_PAIR_CURSOR));
			// Print rem
			wattron(wmaind, COLOR_PAIR(COLOR_PAIR_SELECTION));
			waddstr(wmaind, buf+cur_start+cur_len);
			wattroff(wmaind, A_BOLD | COLOR_PAIR(COLOR_PAIR_SELECTION));
		}else [[ likely ]] {
			waddstr(wmaind, buf);
		}
		delete buf;
	}
}

static const char** HELP_MSG[] = {
		(char const *[]){"Down [j]", "Up [k]", "Next [h]", "Previous [l]", "Modify [\\n]", "Quit [q]", NULL},
		(char const *[]){"Down [j]", "Up [k]", "Select [\\n]", "Cancel [esc]", NULL}
	};
static void tui_render_help(){
	werase(whelpbar);
	for(int i = 0; HELP_MSG[state][i] != NULL; i++){
		waddch(whelpbar, ' ');
		wattron(whelpbar, COLOR_PAIR(COLOR_PAIR_HELP));
		waddstr(whelpbar, HELP_MSG[state][i]);
		wattroff(whelpbar, COLOR_PAIR(COLOR_PAIR_HELP));
	}
}

static bool tui_control_status_menu(const char &c){
	switch(c){
		case 'j':
			menu_driver(cmd_menu, REQ_DOWN_ITEM);
			return true;
		case 'k':
			menu_driver(cmd_menu, REQ_UP_ITEM);
			return true;
		case '\033':
			state = STATE_INITIAL;
			delwin(wmenu);
			tui_redraw();
			return true;
	}
	return false;
}

bool tui_control(const char &c){
	switch(state){
		case STATE_INITIAL:
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
				case 'q':
					abort();
				case '\r':
					state = STATE_CMD_MENU;
					// Items
					const char** const &cmd = Service::cmd[strcmp(services[selection]->status.c_str(), "[started]")];	// remember to changing indexing with selection; ?!
					const int n_cmd = sizeof(*cmd);
					static ITEM** options = (ITEM**)calloc(n_cmd+1, sizeof(ITEM *));
					for(int i = 0; i < n_cmd; i++){
						options[i] = new_item(cmd[i], "");
					}
					options[n_cmd] = NULL;
					// menu win
					const int mstartx = tui_rendered_service_button_pos(services[selection], windoww(wmaind));
					const int mstarty = wmain->_begy + selection+1 + 1;
					const int mwidth = (cursor ? services[selection]->status : services[selection]->runlevel).size() + 2;
					const int mheight = 5;
					wmenu = newwin(mheight, mwidth, mstarty, mstartx);
					wmenud = derwin(wmenu, mheight-2, mwidth-2, 1, 1);
					wborder(wmenu, '|', '|', '=', '=', 'O', 'O', 'O', 'O');
					// menu
					cmd_menu = new_menu(options);
					set_menu_win(cmd_menu, wmenu);
					set_menu_sub(cmd_menu, wmenud);
					post_menu(cmd_menu);
					refresh();

					return true;
			}
			return false;
		case STATE_CMD_MENU:
			tui_control_status_menu(c);
			return false;
	}

	return false;
}

void tui_redraw(){
	box(wmain, 0, 0);
	mvwaddstr(wmain, 0, (scrw-(sizeof(TUI_BANNER)-1))/2, TUI_BANNER);
	//
	wrefresh(wmain);
	if(state == STATE_CMD_MENU){
		wrefresh(wmenu);
	}
	//
	tui_draw();
}

void tui_draw(){
	tui_render_services();
	tui_render_help();
	//
	wrefresh(wmaind);
	if(state == STATE_CMD_MENU){
		wrefresh(wmenu);
		wrefresh(wmenud);
	}
	wrefresh(whelpbar);
}
