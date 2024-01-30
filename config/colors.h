#ifndef COLORS_H
#define COLORS_H

#include <curses.h>

/* List of color options:
			COLOR_BLACK
			COLOR_RED
			COLOR_GREEN
			COLOR_YELLOW
			COLOR_BLUE
			COLOR_MAGENTA
			COLOR_CYAN
			COLOR_WHITE
			-1				// for transparent (only works if that is your default terminal background)
*/

#define COLOR_STD_FG			COLOR_WHITE
#define COLOR_STD_BG			COLOR_BLACK
#define COLOR_SELECTION_FG		COLOR_WHITE
#define COLOR_SELECTION_BG		COLOR_RED
#define COLOR_CURSOR_FG			COLOR_WHITE
#define COLOR_CURSOR_BG			COLOR_BLUE
#define COLOR_HELP_FG			COLOR_BLACK
#define COLOR_HELP_BG			COLOR_WHITE
#define COLOR_WARNING_FG		COLOR_BLACK
#define COLOR_WARNING_BG		COLOR_YELLOW
#define COLOR_ERROR_FG			COLOR_YELLOW
#define COLOR_ERROR_BG			COLOR_WHITE


// ### DO NOT TOUCH !!! ###
enum {
	COLOR_PAIR_STD = 1,
	COLOR_PAIR_SELECTION,
	COLOR_PAIR_CURSOR,
	COLOR_PAIR_HELP,
	COLOR_PAIR_WARNING,
	COLOR_PAIR_ERROR,
};

#define easy_init_pair(x) init_pair(COLOR_PAIR_ ## x, COLOR_ ## x ## _FG, COLOR_ ## x ## _BG)

#endif
