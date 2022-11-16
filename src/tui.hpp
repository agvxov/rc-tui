#pragma once
#include <ncurses.h>
#include "Service.hpp"

extern WINDOW* wmain;
extern WINDOW* whelpbar;

bool tui_init();
void tui_quit();
bool tui_control(const char &c);
void tui_display();
