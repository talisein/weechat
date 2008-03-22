/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_GUI_CURSES_H
#define __WEECHAT_GUI_CURSES_H 1

#ifdef HAVE_NCURSESW_CURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif

#define WINDOW_MIN_WIDTH      10
#define WINDOW_MIN_HEIGHT     5

#define GUI_CURSES(window) ((struct t_gui_curses_objects *)(window->gui_objects))

struct t_gui_bar_window
{
    struct t_gui_bar *bar;          /* pointer to bar                       */
    int x, y;                       /* position of window                   */
    int width, height;              /* window size                          */
    WINDOW *win_bar;                /* bar Curses window                    */
    WINDOW *win_separator;          /* separator (optional)                 */
    struct t_gui_bar_window *next_bar_window;
                                    /* link to next bar window              */
                                    /* (only used if bar is in windows)     */
};

struct t_gui_curses_objects
{
    WINDOW *win_title;              /* title window                         */
    WINDOW *win_chat;               /* chat window (example: channel)       */
    WINDOW *win_nick;               /* nick window                          */
    WINDOW *win_status;             /* status window                        */
    WINDOW *win_infobar;            /* info bar window                      */
    WINDOW *win_input;              /* input window                         */
    WINDOW *win_separator;          /* separation between 2 splited (V) win */
    struct t_gui_bar_window *bar_windows; /* bar windows                    */
};

extern struct t_gui_color gui_weechat_colors[];

/* color functions */
extern int gui_color_get_pair (int num_color);
extern void gui_color_init ();
extern void gui_color_end ();

/* bar functions */
extern void gui_bar_window_calculate_pos_size (struct t_gui_bar_window *bar_window,
                                               struct t_gui_window *window);

/* chat functions */
extern void gui_chat_calculate_line_diff (struct t_gui_window *window,
                                          struct t_gui_line **line,
                                          int *line_pos, int difference);

/* keyboard functions */
extern void gui_keyboard_default_bindings ();
extern int gui_keyboard_read_cb (void *data);

/* window functions */
extern int gui_window_utf_char_valid (char *utf_char);
extern void gui_window_wprintw (WINDOW *window, char *data, ...);
extern void gui_window_curses_clear (WINDOW *window, int num_color);
extern void gui_window_set_weechat_color (WINDOW *window, int num_color);
extern void gui_window_refresh_screen_sigwinch ();
extern void gui_window_title_set ();
extern void gui_window_title_reset ();

#endif /* gui-curses.h */
