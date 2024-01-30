#ifndef TUI_HPP
#define TUI_HPP

extern bool tui_init();
extern void tui_quit();
extern bool tui_control(const int &c);
extern void tui_redraw();
extern void tui_draw();
extern inline void tui_resize();

#endif
