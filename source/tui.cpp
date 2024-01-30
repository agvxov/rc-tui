#include "tui.hpp"
#include <time.h>
#include <algorithm>
#include <ncurses.h>
#include <menu.h>
#include <readline/readline.h>

#include "Service.hpp"
#include "search.hpp"

#include "config/colors.h"

#ifndef ESC
# define ESC '\033'
#endif
#ifndef ENTER
# define ENTER '\r'
#endif

#define TUI_BANNER "| Open-rc TUI |"

extern bool is_root;

template<typename T>
struct array {
	T* elements;
	int size;

	array() {
	}

	array(int n) : size(n) {
		elements = new T[n];
	}

	~array() {
		delete[] this->elements;
	}

    T& operator[](int i) {
        return elements[i];
    }
};

static bool tui_running;

enum {
	CUR_RUNLEVEL,
	CUR_STATUS,
	CUR_ENUM_END,
};
static size_t cursor    = CUR_STATUS;
static size_t selection = 0;

enum {
	STATE_INITIAL,
	STATE_SEARCH,
	STATE_CMD_MENU,
};
static size_t state = STATE_INITIAL;

static WINDOW * wmain;
static WINDOW * wmaind;
static WINDOW * wmenu;
static WINDOW * wmenud;
static WINDOW * whelpbar;
static WINDOW * wsearchbar;
static MENU   * cmd_menu;

static array<ITEM*> runlevel_options;
static array<ITEM*> status_options[2];

static menu_callback_t menu_callback;


static inline size_t windoww(WINDOW* w){
	return (w->_maxx+1 - w->_begx)+1;
}
static inline size_t windowh(WINDOW* w){
	return (w->_maxy+1 - w->_begy+w->_yoffset)+1;
}

static size_t top = 0;

static int input_available = false;
static char input;

inline int option_list_to_items(ITEM** &items, const char * const * const &options) {
	int n = 0;
	while(options[n] != NULL) {
		++n;
	}
	items = (ITEM**)malloc((n+1) * sizeof(ITEM*));
	for (int i = 0; i < n; i++) {
		items[i] = new_item(options[i], "");
	}
	items[n] = NULL;
	return n;
}

bool tui_init(){
	// Prerequizet info
	for(auto i : services){
		if(i->name.size() > SERVICE_MAX_NAME_LEN){
			SERVICE_MAX_NAME_LEN = i->name.size();
		}
	}

	runlevel_options.size  = option_list_to_items(runlevel_options.elements,  Service::runlevels);
	status_options[0].size = option_list_to_items(status_options[0].elements, Service::cmd[0]);
	status_options[1].size = option_list_to_items(status_options[1].elements, Service::cmd[1]);

	// NCurses
	initscr();

	nonl();
	noecho();
	curs_set(0);

	start_color();
	easy_init_pair(STD);
	easy_init_pair(SELECTION);
	easy_init_pair(CURSOR);
	easy_init_pair(HELP);
	easy_init_pair(WARNING);
	easy_init_pair(ERROR);

	wmain      = newwin(LINES-2, COLS, 0, 0);
	wmaind     = derwin(wmain, wmain->_maxy-1, wmain->_maxx-1, 1, 1);
	whelpbar   = newwin(1, COLS, LINES-1, 0);
	wsearchbar = newwin(1, COLS, LINES-2, 0);

	refresh();

	wbkgd(wmain, COLOR_PAIR(COLOR_PAIR_STD));
	wbkgd(wmaind, COLOR_PAIR(COLOR_PAIR_STD));
	wbkgd(whelpbar, COLOR_PAIR(COLOR_PAIR_STD));
	wbkgd(wsearchbar, COLOR_PAIR(COLOR_PAIR_STD));

	// ReadLine
	rl_bind_key('\t', rl_insert);	// make tab insert itself
	rl_catch_signals = 0;			// do not install signal handlers
	rl_catch_sigwinch = 0;			// do not care about window change signals
	rl_prep_term_function = NULL;	// do not initialize the ternimal
	rl_deprep_term_function = NULL; // do not clean up
	rl_change_environment = 0;		// ?!
	rl_getc_function = +[]([[ maybe_unused ]] FILE* ignore){
		input_available = false;
		return (int)input;
	};
	rl_input_available_hook = +[]{
		return input_available;
	};
	rl_redisplay_function = +[]{
		wmove(wsearchbar, 0, 1);
		wclrtoeol(wsearchbar);
		waddstr(wsearchbar, rl_line_buffer);
		wrefresh(wsearchbar);
		return;
	};
	rl_callback_handler_install("", +[]([[ maybe_unused ]] char *line){
		return;
	});

	tui_running = true;
	return true;
}

void tui_quit(){
	if(not tui_running){ return; }

	delwin(wsearchbar);
	delwin(whelpbar);
	delwin(wmenud);
	delwin(wmenu);
	delwin(wmaind);
	delwin(wmain);
	endwin();

	tui_running = false;
}

static char* tui_render_service(const Service* const s, const size_t &width){
	char* r;

	static const char l[] = "   Locked";
	auto l_spaceout = []() constexpr {
										char* const r = (char*)malloc(sizeof(l));
										memset(r, ' ', sizeof(l)-1);
										r[sizeof(l)-1] = '\00';
										return r;
									};
	const char* const lstr = (s->locked ? l : l_spaceout());

	int err = asprintf(
				&r,
				"%-*s%s%*s%*s%*s",	/* name,<locked>,<space_padding>,runlevel,status */
				(int)SERVICE_MAX_NAME_LEN,
				s->name.c_str(),
				lstr,
				(int)(width-
						(SERVICE_MAX_NAME_LEN
							+ (sizeof(l)-1)
							+ SERVICE_MAX_RUNLEVEL_LEN
							+ SERVICE_MAX_STATUS_LEN+1)
						),
				" ",
				SERVICE_MAX_RUNLEVEL_LEN,
				s->runlevel.c_str(),
				SERVICE_MAX_STATUS_LEN+1,
				s->status.c_str()
			);

	if (err == -1) {
		abort();
	}

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
	const size_t t = std::min(service_results.size() - top, (size_t)windowh(wmaind));
	for(size_t i = top; i < (top + t); i++){
		char* buf = tui_render_service(service_results[i], winw);
		if(i == selection) [[ unlikely ]] {
			size_t cur_start,
				cur_len;
			cur_start = tui_rendered_service_button_pos(service_results[i], winw);
			switch(cursor){
				case CUR_RUNLEVEL:
					cur_len = service_results[i]->runlevel.size();
					break;
				case CUR_STATUS:
					cur_len = service_results[i]->status.size();
					break;
			}
			// Print until cursor
			wattron(wmaind, A_BOLD | COLOR_PAIR(COLOR_PAIR_SELECTION));
			waddnstr(wmaind, buf, cur_start);
			wattroff(wmaind, COLOR_PAIR(COLOR_PAIR_SELECTION));
			// Print cursor
			wattron(wmaind, COLOR_PAIR(COLOR_PAIR_CURSOR));
			waddnstr(wmaind, buf+cur_start, cur_len);
			wattroff(wmaind, COLOR_PAIR(COLOR_PAIR_CURSOR));
			// Print remainder
			wattron(wmaind, COLOR_PAIR(COLOR_PAIR_SELECTION));
			waddstr(wmaind, buf+cur_start+cur_len);
			wattroff(wmaind, A_BOLD | COLOR_PAIR(COLOR_PAIR_SELECTION));
		}else [[ likely ]] {
			waddstr(wmaind, buf);
		}
		delete buf;
	}
	if ((service_results.size() - top) < windowh(wmaind)) {
		wclrtobot(wmaind);
	}
}

static const char NOT_ROOT_MSG[] = " WARNING: NOT RUNNING AS ROOT! ";
static const char * const * const HELP_MSG[] = {
		[STATE_INITIAL]  = (char const *[]){"Down [j]", "Up [k]", "Next [h]", "Previous [l]", "Modify [\\n]", "Search [/]", "Quit [q]", NULL},
		[STATE_SEARCH]   = (char const *[]){"Browse [\\n]", "Cancel [ESC]", NULL},
		[STATE_CMD_MENU] = (char const *[]){"Down [j]", "Up [k]", "Select [\\n]", "Cancel [ESC]", NULL},
	};
static void tui_render_help(){
	werase(whelpbar);
	for(int i = 0; HELP_MSG[state][i] != NULL; i++){
		waddch(whelpbar, ' ');
		wattron(whelpbar, COLOR_PAIR(COLOR_PAIR_HELP));
		waddstr(whelpbar, HELP_MSG[state][i]);
		wattroff(whelpbar, COLOR_PAIR(COLOR_PAIR_HELP));
	}

	if (not is_root) {
		wattron(whelpbar, COLOR_PAIR(COLOR_PAIR_WARNING));
		mvwaddstr(whelpbar, 0, COLS - sizeof(NOT_ROOT_MSG), NOT_ROOT_MSG);
		wattroff(whelpbar, COLOR_PAIR(COLOR_PAIR_WARNING));
	}
}

static inline void make_menu(const int w, int y, const int x, const int flip_above, array<ITEM*> &items, menu_callback_t callback) {
	menu_callback = callback;

	if (flip_above < items.size+2) {
		y = y - (items.size+2) - 1;
	}

	wmenu  = newwin(items.size+2, w, y, x);
	wmenud = derwin(wmenu, items.size, w-2, 1, 1);

	wbkgd(wmenu, COLOR_PAIR(COLOR_PAIR_STD));
	wbkgd(wmenud, COLOR_PAIR(COLOR_PAIR_STD));

	cmd_menu = new_menu(items.elements);

	set_menu_win(cmd_menu, wmenu);
	set_menu_sub(cmd_menu, wmenud);
	post_menu(cmd_menu);
}

static inline bool tui_control_search(const char &c){
	switch(c) {
		case ESC: {
			state = STATE_INITIAL;
			rl_end   = 0;
			rl_point = 0;
			wmove(wsearchbar, 0, 0);
			wclrtoeol(wsearchbar);
			wrefresh(wsearchbar);
			service_results.resize(services.size());
			copy(services.begin(), services.end(), service_results.begin());
		} return true;
		case ENTER: {
			state = STATE_INITIAL;
		} return true;
		default: {
			input = c;
			input_available = true;
			int cache = rl_end;
			rl_callback_read_char();
			if (cache > rl_end) { // erasing occured
				service_results.resize(services.size());
				copy(services.begin(), services.end(), service_results.begin());
			}
			search(service_results, rl_line_buffer);
			if (selection > service_results.size()-1) {
				selection = service_results.size()-1;
			}
		} return true;
	}

	return false;
}

static inline bool tui_control_menu(const char &c){
	switch(c) {
		case 'j':
			menu_driver(cmd_menu, REQ_DOWN_ITEM);
			return true;
		case 'k':
			menu_driver(cmd_menu, REQ_UP_ITEM);
			return true;
		case ESC:
			state = STATE_INITIAL;
			delwin(wmenu);
			tui_redraw();
			return true;
		case ENTER:
			(services[selection]->*menu_callback)(item_index(current_item(cmd_menu)));
			state = STATE_INITIAL;
			free_menu(cmd_menu);
			delwin(wmenud);
			delwin(wmenu);
			tui_redraw();
			return true;
	}
	return false;
}

static inline bool tui_control_root(const char &c) {
	switch(c){
		case 'j':
			if (selection < services.size()-1) {
				++selection;
				if (selection >= (top + windowh(wmaind))) {
					++top;
				}
			}else{
				selection = 0;
				top       = 0;
			}
			return true;
		case 'k':
			if (selection > 0) {
				if (selection == top) {
					--top;
				}
				--selection;
			} else {
				if (top > 0) {
					--top;
				} else {
					selection = services.size()-1;
					top       = services.size() - windowh(wmaind);
				}
			}
			return true;
		case 'h':
			if (cursor != 0) {
				--cursor;
			} else {
				cursor = CUR_ENUM_END-1;
			}
			return true;
		case 'l':
			++cursor;
			if (cursor == CUR_ENUM_END) {
				cursor = 0;
			}
			return true;
		case 'q':
			abort();
		case '/':
		case '?':
			state = STATE_SEARCH;
			mvwaddch(wsearchbar, 0, 0, '/');
			return true;
		case '\r':
			if (not is_root) {
				wattron(whelpbar, COLOR_PAIR(COLOR_PAIR_ERROR));
				mvwaddstr(whelpbar, 0, COLS - sizeof(NOT_ROOT_MSG), NOT_ROOT_MSG);
				wattroff(whelpbar, COLOR_PAIR(COLOR_PAIR_ERROR));
				wrefresh(whelpbar);
				refresh();
				auto tmp = (const struct timespec){.tv_sec = 0, .tv_nsec = 100'000'000};
				nanosleep(&tmp, NULL);
				flushinp();
				return true;
			}
			state = STATE_CMD_MENU;
			array<ITEM*> * options;
			if (cursor == CUR_RUNLEVEL) {
				options = &runlevel_options;
			} else if (cursor == CUR_STATUS) {
				options = &status_options[services[selection]->status != "[started]"];
			}
			const int mstartx = tui_rendered_service_button_pos(services[selection], windoww(wmaind));
			const int mstarty = wmaind->_begy + selection+1;
			const int mwidth  = (
								cursor
									?
								services[selection]->status
									:
								services[selection]->runlevel
							).size() + 2;
			const int lines_avail = windowh(wmaind) - (selection - top);
			make_menu(mwidth, mstarty, mstartx, lines_avail, *options, &Service::change_status);

			return true;
	}
	return false;
}

bool tui_control(const int &c){
	if (c == KEY_RESIZE) {
		tui_resize();
		flushinp();
		return true;
	}

	switch(state){
		case STATE_INITIAL:
			return tui_control_root(c);
		case STATE_SEARCH:
			return tui_control_search(c);
		case STATE_CMD_MENU:
			return tui_control_menu(c);
	}

	return false;
}

void tui_redraw(){
	box(wmain, 0, 0);
	mvwaddstr(wmain, 0, (COLS-(sizeof(TUI_BANNER)-1))/2, TUI_BANNER);
	wrefresh(wmain);

	tui_draw();
}

void tui_draw(){
	tui_render_services();
	wrefresh(wmaind);

	tui_render_help();
	wrefresh(whelpbar);
	
	if(state == STATE_CMD_MENU){
		wborder(wmenu, '|', '|', '=', '=', 'O', 'O', 'O', 'O');
		wrefresh(wmenu);
		wrefresh(wmenud);
	}

	if (state == STATE_SEARCH) {
		wrefresh(wsearchbar);
	}

	refresh();
}

inline void tui_resize() {
	tui_quit();
	tui_init();
	tui_redraw();
}
