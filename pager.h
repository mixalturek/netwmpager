/* 
 * Copyright 2004 Timo Hirvonen
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _PAGER_H
#define _PAGER_H

#include <x.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

struct pager;

enum pager_layer {
	LAYER_BELOW,
	LAYER_NORMAL,
	LAYER_ABOVE
};

/* colors. format: "rgb:80/ff/80" */
extern char *active_win_color;
extern char *inactive_win_color;
extern char *active_desk_color;
extern char *inactive_desk_color;
extern char *win_border_color;
extern char *grid_color;

extern char *active_win_font_color;
extern char *inactive_win_font_color;

extern char *popup_color;
extern char *popup_font_color;

/* extern struct pager *pager_new(const char *geometry, int wait_wm); */
extern struct pager *pager_new(const char *geometry, int cols, int rows);
extern void pager_delete(struct pager *pager);

/* events */
extern void pager_expose_event(struct pager *pager, XEvent *event);
extern void pager_configure_notify(struct pager *pager);
extern void pager_property_notify(struct pager *pager);

/* flush events (see above) */
extern void pager_handle_events(struct pager *pager);

/* options */
extern void pager_set_opacity(struct pager *pager, double opacity);
extern void pager_set_layer(struct pager *pager, enum pager_layer layer);
extern int pager_set_window_font(struct pager *pager, const char *name);
extern int pager_set_popup_font(struct pager *pager, const char *name);
extern void pager_set_show_sticky(struct pager *pager, int on);
extern void pager_set_show_window_titles(struct pager *pager, int on);
extern void pager_set_show_window_icons(struct pager *pager, int on);
extern void pager_set_show_popups(struct pager *pager, int on);
extern void pager_set_allow_cover(struct pager *pager, int on);

extern void pager_show(struct pager *pager);

extern void pager_button_press(struct pager *pager, int x, int y, int button);
extern void pager_button_release(struct pager *pager, int x, int y, int button);
extern void pager_motion(struct pager *pager, int x, int y);
extern void pager_enter(struct pager *pager, int x, int y);
extern void pager_leave(struct pager *pager);

#endif
