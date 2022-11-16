#pragma once
#include <ncurses.h>
#include <menu.h>
#include "Service.hpp"

bool tui_init();
void tui_quit();
bool tui_control(const char &c);
void tui_redraw();
void tui_draw();
