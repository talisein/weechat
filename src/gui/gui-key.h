/*
 * Copyright (C) 2003-2011 Sebastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_GUI_KEY_H
#define __WEECHAT_GUI_KEY_H 1

#define GUI_KEY_BUFFER_BLOCK_SIZE 256

#define GUI_KEY_GRAB_DELAY_DEFAULT 500

enum t_gui_key_context
{
    GUI_KEY_CONTEXT_DEFAULT = 0,
    GUI_KEY_CONTEXT_SEARCH,
    GUI_KEY_CONTEXT_CURSOR,
    GUI_KEY_CONTEXT_MOUSE,
    /* number of key contexts */
    GUI_KEY_NUM_CONTEXTS,
};

/* key structures */

struct t_gui_key
{
    char *key;                      /* key combo (ex: a, ^W, ^W^C, meta-a)  */
    char *command;                  /* associated command (may be NULL)     */
    struct t_gui_key *prev_key;     /* link to previous key                 */
    struct t_gui_key *next_key;     /* link to next key                     */
};

/* key variables */

extern struct t_gui_key *gui_keys[GUI_KEY_NUM_CONTEXTS];
extern struct t_gui_key *last_gui_key[GUI_KEY_NUM_CONTEXTS];
extern struct t_gui_key *gui_default_keys[GUI_KEY_NUM_CONTEXTS];
extern struct t_gui_key *last_gui_default_key[GUI_KEY_NUM_CONTEXTS];
extern int gui_keys_count[GUI_KEY_NUM_CONTEXTS];
extern int gui_default_keys_count[GUI_KEY_NUM_CONTEXTS];
extern char *gui_key_context_string[GUI_KEY_NUM_CONTEXTS];
extern int gui_key_verbose;
extern char gui_key_combo_buffer[];
extern int gui_key_grab;
extern int gui_key_grab_count;
extern int *gui_key_buffer;
extern int gui_key_buffer_size;
extern int gui_key_paste_pending;
extern time_t gui_key_last_activity_time;

/* key functions */

extern void gui_key_init ();
extern int gui_key_search_context (const char *context);
extern void gui_key_grab_init (int grab_command, const char *delay);
extern char *gui_key_get_internal_code (const char *key);
extern char *gui_key_get_expanded_name (const char *key);
extern struct t_gui_key *gui_key_new (struct t_gui_buffer *buffer,
                                      int context,
                                      const char *key,
                                      const char *command);
extern struct t_gui_key *gui_key_search (struct t_gui_key *keys,
                                         const char *key);
extern struct t_gui_key *gui_key_bind (struct t_gui_buffer *buffer,
                                       int context,
                                       const char *key,
                                       const char *command);
extern int gui_key_unbind (struct t_gui_buffer *buffer, int context,
                           const char *key, int send_signal);
extern int gui_key_focus (const char *key, int context);
extern int gui_key_pressed (const char *key_str);
extern void gui_key_free (struct t_gui_key **keys,
                          struct t_gui_key **last_key,
                          int *keys_count,
                          struct t_gui_key *key);
extern void gui_key_free_all (struct t_gui_key **keys,
                              struct t_gui_key **last_key,
                              int *keys_count);
extern void gui_key_buffer_reset ();
extern void gui_key_buffer_add (unsigned char key);
extern int gui_key_get_paste_lines ();
extern void gui_key_paste_accept ();
extern void gui_key_paste_cancel ();
extern void gui_key_end ();
extern struct t_hdata *gui_key_hdata_key_cb (void *data,
                                             const char *hdata_name);
extern int gui_key_add_to_infolist (struct t_infolist *infolist,
                                    struct t_gui_key *key);
extern void gui_key_print_log (struct t_gui_buffer *buffer);

/* key functions (GUI dependent) */

extern void gui_key_default_bindings ();

#endif /* __WEECHAT_GUI_KEY_H */