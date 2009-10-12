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

#ifndef _X_H
#define _X_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>

enum atom_index {
	UTF8_STRING,
	_XROOTPMAP_ID,
	WM_NAME,
	WM_ICON_NAME,
	_NET_ACTIVE_WINDOW,
	_NET_CLIENT_LIST,
	_NET_CLIENT_LIST_STACKING,
	_NET_CURRENT_DESKTOP,
	_NET_DESKTOP_GEOMETRY,
	_NET_DESKTOP_LAYOUT,
	_NET_DESKTOP_NAMES,
	_NET_DESKTOP_VIEWPORT,
	_NET_NUMBER_OF_DESKTOPS,
	_NET_SHOWING_DESKTOP,
	_NET_SUPPORTED,
	_NET_SUPPORTING_WM_CHECK,
	_NET_VIRTUAL_ROOTS,
	_NET_WM_ALLOWED_ACTIONS,
	_NET_WM_DESKTOP,
	_NET_WM_HANDLED_ICONS,
	_NET_WM_ICON,
	_NET_WM_ICON_GEOMETRY,
	_NET_WM_ICON_NAME,
	_NET_CLOSE_WINDOW,
	_NET_WM_MOVERESIZE,
	_NET_WM_NAME,
	_NET_WM_PID,
	_NET_WM_STATE,

	_NET_WM_STATE_MODAL,
	/* NOTE: this is not same as desktop = -1 */
	_NET_WM_STATE_STICKY,
	_NET_WM_STATE_MAXIMIZED_VERT,
	_NET_WM_STATE_MAXIMIZED_HORZ,
	_NET_WM_STATE_SHADED,
	_NET_WM_STATE_SKIP_TASKBAR,
	_NET_WM_STATE_SKIP_PAGER,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_ABOVE,
	_NET_WM_STATE_BELOW,
	_NET_WM_STATE_DEMANDS_ATTENTION,
	/* openbox */
	_OB_WM_STATE_UNDECORATED,

	_NET_WM_STRUT,
	_NET_WM_STRUT_PARTIAL,
	_NET_WM_USER_TIME,
	_NET_WM_VISIBLE_ICON_NAME,
	_NET_WM_VISIBLE_NAME,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DESKTOP,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_TOOLBAR,
	_NET_WM_WINDOW_TYPE_MENU,
	_NET_WM_WINDOW_TYPE_UTILITY,
	_NET_WM_WINDOW_TYPE_SPLASH,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_WM_WINDOW_TYPE_NORMAL,
	_NET_WM_WINDOW_OPACITY,
	_NET_WORKAREA,
	NR_ATOMS
};

#define WINDOW_STATE_MODAL		(1 << 0)
#define WINDOW_STATE_STICKY		(1 << 1)
#define WINDOW_STATE_MAXIMIZED_VERT	(1 << 2)
#define WINDOW_STATE_MAXIMIZED_HORZ	(1 << 3)
#define WINDOW_STATE_SHADED		(1 << 4)
#define WINDOW_STATE_SKIP_TASKBAR	(1 << 5)
#define WINDOW_STATE_SKIP_PAGER		(1 << 6)
#define WINDOW_STATE_HIDDEN		(1 << 7)
#define WINDOW_STATE_FULLSCREEN		(1 << 8)
#define WINDOW_STATE_ABOVE		(1 << 9)
#define WINDOW_STATE_BELOW		(1 << 10)
#define WINDOW_STATE_DEMANDS_ATTENTION	(1 << 11)

enum state_action {
	/* unset propery */
	_NET_WM_STATE_REMOVE,
	/* set property */
	_NET_WM_STATE_ADD,
	/* toggle property */
	_NET_WM_STATE_TOGGLE
};

enum window_type {
	WINDOW_TYPE_DESKTOP,
	WINDOW_TYPE_DOCK,
	WINDOW_TYPE_TOOLBAR,
	WINDOW_TYPE_MENU,
	WINDOW_TYPE_UTILITY,
	WINDOW_TYPE_SPLASH,
	WINDOW_TYPE_DIALOG,
	WINDOW_TYPE_NORMAL
};

enum strut_type {
	STRUT_TYPE_LEFT,
	STRUT_TYPE_RIGHT,
	STRUT_TYPE_TOP,
	STRUT_TYPE_BOTTOM
};

enum source_indication {
	SOURCE_INDICATION_NONE,
	SOURCE_INDICATION_NORMAL,
	SOURCE_INDICATION_PAGER
};

extern Display *display;

extern int x_init(const char *display_name);
extern void x_exit(void);

/* cached XInternAtom */
extern Atom x_get_atom(enum atom_index idx);

extern int x_get_property(Window window, Atom type, Atom property, size_t type_size, void **prop_ret, int *nr_ret);
extern int x_get_property_nr(Window window, Atom type, Atom property, size_t type_size, void *prop_ret, int nr);

static inline int x_get_atom_property(Window window, Atom property, Atom **prop_ret, int *nr_ret)
{
	return x_get_property(window, XA_ATOM, property, sizeof(Atom), (void **)prop_ret, nr_ret);
}

static inline int x_get_atom_property_nr(Window window, Atom property, Atom *prop_ret, int nr)
{
	return x_get_property_nr(window, XA_ATOM, property, sizeof(Atom), prop_ret, nr);
}

static inline int x_get_cardinal_property(Window window, Atom property, unsigned long **prop_ret, int *nr_ret)
{
	return x_get_property(window, XA_CARDINAL, property, sizeof(unsigned long), (void **)prop_ret, nr_ret);
}

static inline int x_get_cardinal_property_nr(Window window, Atom property, unsigned long *prop_ret, int nr)
{
	return x_get_property_nr(window, XA_CARDINAL, property, sizeof(unsigned long), prop_ret, nr);
}

static inline int x_get_window_property(Window window, Atom property, Window **prop_ret, int *nr_ret)
{
	return x_get_property(window, XA_WINDOW, property, sizeof(Window), (void **)prop_ret, nr_ret);
}

static inline int x_get_window_property_nr(Window window, Atom property, Window *prop_ret, int nr)
{
	return x_get_property_nr(window, XA_WINDOW, property, sizeof(Window), prop_ret, nr);
}

extern int x_get_string_property(Window window, Atom property, char **prop_ret);
extern int x_get_string_array_property(Window window, Atom property, char ***prop_ret, int *nr_ret);
extern int x_get_utf8_string_property(Window window, Atom property, char **prop_ret);
extern int x_get_utf8_string_array_property(Window window, Atom property, char ***prop_ret, int *nr_ret);

extern int x_set_property(Window window, Atom type, Atom property, int format, const void *prop, int nr);

static inline int x_set_atom_property(Window window, Atom property, const Atom *prop, int nr)
{
	return x_set_property(window, XA_ATOM, property, 32, prop, nr);
}

static inline int x_set_cardinal_property(Window window, Atom property, const unsigned long *prop, int nr)
{
	return x_set_property(window, XA_CARDINAL, property, 32, prop, nr);
}

static inline int x_set_string_property(Window window, Atom property, const char *prop)
{
	return x_set_property(window, XA_STRING, property, 8, prop, strlen(prop));
}

static inline int x_set_utf8_string_property(Window window, Atom property, const char *prop)
{
	return x_set_property(window, x_get_atom(UTF8_STRING), property, 8, prop, strlen(prop));
}

extern int x_is_netwm_compatible_wm_running(void);
extern int x_get_wm_name(char **wm_name);

extern int x_get_client_list(int stacking, Window **windows, int *nr_windows);
extern int x_get_client_list_for_desktop(int stacking, int desktop, Window **windows, int *nr_windows);

extern int x_set_desktop_layout(int columns, int rows);
extern int x_get_desktop_layout(int *columns, int *rows);

extern int x_get_desktop_names(char ***namesp);

extern int x_set_showing_desktop(int on);
extern int x_get_showing_desktop(int *on);

extern int x_set_current_desktop(int desktop);
extern int x_get_current_desktop(int *desktop);

extern int x_set_active_window(Window window, enum source_indication si);
extern int x_get_active_window(Window *window);

extern int x_get_root_pixmap(Pixmap *pixmap);

/* desktop = 0-N, -1 = all desktops */
extern int x_window_set_desktop(Window window, int desktop);
extern int x_window_get_desktop(Window window, int *desktop);

extern int x_window_set_title(Window window, const char *title);
extern int x_window_get_title(Window window, char **title);

extern int x_window_set_geometry(Window window, int flags, int x, int y, unsigned int w, unsigned int h);
extern int x_window_get_geometry(Window window, int *x, int *y, int *w, int *h);

extern int x_window_set_type(Window window, enum window_type type);
extern int x_window_get_type(Window window, enum window_type *type);

extern int x_window_set_strut_partial(Window window, enum strut_type, int size, int start, int end);

/* opacity = 0.0 - 1.0 */
extern int x_window_set_opacity(Window window, double opacity);
extern int x_window_get_opacity(Window window, double *opacity);

/* set 1-2 _NET_WM_STATE_* states for @window. if @prop2 is 0 then only prop1 is set */
extern int x_window_set_states(Window window, enum state_action action, enum atom_index prop1, enum atom_index prop2);
extern int x_window_get_states(Window window, unsigned int *states);

extern int x_window_get_icon(Window window, int *width, int *height, char **data);

extern int x_window_set_aspect(Window window, int x, int y);

extern int x_window_close(Window window);
extern int x_window_move_resize(Window window, unsigned long x_root_button, unsigned long y_root_button, unsigned long direction, unsigned long button);

/* returns 1 if found, 0 if not, <0 on error */
extern int x_get_window_by_name(const char *name, Window *window);

extern int x_window_set_modal(Window window, enum state_action action);
/* NOTE: this is not same as desktop = -1 */
extern int x_window_set_sticky(Window window, enum state_action action);
extern int x_window_set_maximized_vert(Window window, enum state_action action);
extern int x_window_set_maximized_horz(Window window, enum state_action action);
extern int x_window_set_shaded(Window window, enum state_action action);
extern int x_window_set_skip_taskbar(Window window, enum state_action action);
extern int x_window_set_skip_pager(Window window, enum state_action action);
extern int x_window_set_hidden(Window window, enum state_action action);
extern int x_window_set_fullscreen(Window window, enum state_action action);
extern int x_window_set_above(Window window, enum state_action action);
extern int x_window_set_below(Window window, enum state_action action);
extern int x_window_set_demands_attention(Window window, enum state_action action);
extern int x_window_set_maximized(Window window, enum state_action action);
extern int x_window_set_undecorated(Window window, enum state_action action);

static inline void x_parse_geometry(const char *str, int *flags, int *x, int *y, unsigned int *w, unsigned int *h)
{
	*flags = XParseGeometry(str, x, y, w, h);
}

extern int x_parse_color(const char *name, unsigned long *color);

#endif
