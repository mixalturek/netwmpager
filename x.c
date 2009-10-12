/* 
 * Copyright 2004-2005 Timo Hirvonen
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

#include <x.h>
#include <xmalloc.h>
#include <debug.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <string.h>

/* orientation of pager */
enum {
	_NET_WM_ORIENTATION_HORZ,
	_NET_WM_ORIENTATION_VERT
};

/* starting corner for counting spaces */
enum {
	_NET_WM_TOPLEFT,
	_NET_WM_TOPRIGHT,
	_NET_WM_BOTTOMRIGHT,
	_NET_WM_BOTTOMLEFT
};

static const char *atom_names[NR_ATOMS] = {
	"UTF8_STRING",
	"_XROOTPMAP_ID",
	"WM_NAME",
	"WM_ICON_NAME",
	"_NET_ACTIVE_WINDOW",
	"_NET_CLIENT_LIST",
	"_NET_CLIENT_LIST_STACKING",
	"_NET_CURRENT_DESKTOP",
	"_NET_DESKTOP_GEOMETRY",
	"_NET_DESKTOP_LAYOUT",
	"_NET_DESKTOP_NAMES",
	"_NET_DESKTOP_VIEWPORT",
	"_NET_NUMBER_OF_DESKTOPS",
	"_NET_SHOWING_DESKTOP",
	"_NET_SUPPORTED",
	"_NET_SUPPORTING_WM_CHECK",
	"_NET_VIRTUAL_ROOTS",
	"_NET_WM_ALLOWED_ACTIONS",
	"_NET_WM_DESKTOP",
	"_NET_WM_HANDLED_ICONS",
	"_NET_WM_ICON",
	"_NET_WM_ICON_GEOMETRY",
	"_NET_WM_ICON_NAME",
	"_NET_CLOSE_WINDOW",
	"_NET_WM_MOVERESIZE",
	"_NET_WM_NAME",
	"_NET_WM_PID",
	"_NET_WM_STATE",

	"_NET_WM_STATE_MODAL",
	"_NET_WM_STATE_STICKY",
	"_NET_WM_STATE_MAXIMIZED_VERT",
	"_NET_WM_STATE_MAXIMIZED_HORZ",
	"_NET_WM_STATE_SHADED",
	"_NET_WM_STATE_SKIP_TASKBAR",
	"_NET_WM_STATE_SKIP_PAGER",
	"_NET_WM_STATE_HIDDEN",
	"_NET_WM_STATE_FULLSCREEN",
	"_NET_WM_STATE_ABOVE",
	"_NET_WM_STATE_BELOW",
	"_NET_WM_STATE_DEMANDS_ATTENTION",
	/* openbox */
	"_OB_WM_STATE_UNDECORATED",

	"_NET_WM_STRUT",
	"_NET_WM_STRUT_PARTIAL",
	"_NET_WM_USER_TIME",
	"_NET_WM_VISIBLE_ICON_NAME",
	"_NET_WM_VISIBLE_NAME",
	"_NET_WM_WINDOW_TYPE",
	"_NET_WM_WINDOW_TYPE_DESKTOP",
	"_NET_WM_WINDOW_TYPE_DOCK",
	"_NET_WM_WINDOW_TYPE_TOOLBAR",
	"_NET_WM_WINDOW_TYPE_MENU",
	"_NET_WM_WINDOW_TYPE_UTILITY",
	"_NET_WM_WINDOW_TYPE_SPLASH",
	"_NET_WM_WINDOW_TYPE_DIALOG",
	"_NET_WM_WINDOW_TYPE_NORMAL",
	"_NET_WM_WINDOW_OPACITY",
	"_NET_WORKAREA"
};

static Atom atom_values[NR_ATOMS] = { 0, };

Display *display = NULL;


/*
 * @to_window: to what window event is sent
 * @window:    window parameter or 0
 * @atom_idx:  message type
 */
static int client_msg(Window to_window, Window window,
		enum atom_index atom_idx, unsigned long data0,
		unsigned long data1, unsigned long data2,
		unsigned long data3, unsigned long data4)
{
	XEvent event;

	event.xclient.type = ClientMessage;
	event.xclient.serial = 0;
	event.xclient.send_event = True;
	event.xclient.window = window;
	event.xclient.display = display;
	event.xclient.message_type = x_get_atom(atom_idx);
	event.xclient.format = 32;

	event.xclient.data.l[0] = data0;
	event.xclient.data.l[1] = data1;
	event.xclient.data.l[2] = data2;
	event.xclient.data.l[3] = data3;
	event.xclient.data.l[4] = data4;

	if (XSendEvent(display, to_window, False,
				SubstructureNotifyMask | SubstructureRedirectMask,
				&event)) {
		return 0;
	} else {
		d_print("Cannot send %d event.\n", (int)x_get_atom(atom_idx));
		return 1;
	}
}

static int get_property_array(Window window, Atom type, Atom property, char **prop_ret, int *nr_ret)
{
	int format;
	unsigned long nr, bytes;
	Atom ret_type;
	int rc;

	rc = XGetWindowProperty(display, window,
			property,
			0, 32 * 1024, False,
			type, &ret_type, &format, &nr, &bytes,
			(unsigned char **)prop_ret);
	if (rc != Success) {
		d_print("XGetWindowProperty: window: 0x%x, property: 0x%x, return code: %d\n",
				(unsigned int)window,
				(unsigned int)property, rc);
		return -2;
	}
	if (type != ret_type) {
		if (ret_type != 0)
			d_print("window 0x%x: property: %d: %d (type) != %d (ret_type)\n",
					(int)window, (int)property,
					(int)type, (int)ret_type);
		/* don't XFree(prop_ret) */
		return -1;
	}
	*nr_ret = nr;
	return 0;
}

static int get_str_property(Window window, Atom str_type, Atom property, char **prop_ret)
{
	int rc, n;
	char *p;

	rc = get_property_array(window, str_type, property, &p, &n);
	if (rc)
		return rc;
	*prop_ret = xstrdup(p);
	XFree(p);
	return 0;
}

static int get_str_array_property(Window window, Atom str_type, Atom property, char ***prop_ret, int *nr_ret)
{
	int rc, n, i, count;
	char *p, *ptr, **a;

	rc = get_property_array(window, str_type, property, &p, &n);
	if (rc)
		return rc;

	count = 0;
	for (i = 0; i < n; i++) {
		if (p[i] == 0)
			count++;
	}

	a = xnew(char *, count + 1);
	ptr = p;
	for (i = 0; i < count; i++) {
		a[i] = xstrdup(ptr);
		ptr += strlen(ptr) + 1;
		d_print("'%s'\n", a[i]);
	}
	a[i] = NULL;
	XFree(p);

	*prop_ret = a;
	*nr_ret = count;
	return 0;
}

static int x_window_find_top_parent(Window window, Window *top)
{
	while (1) {
		Window root, parent;
		Window *c;
		unsigned int n;

		if (!XQueryTree(display, window, &root, &parent, &c, &n))
			return -1;
		if (c)
			XFree(c);
		if (parent == root) {
			*top = window;
			return 0;
		}
		window = parent;
	}
}

int x_init(const char *display_name)
{
	display = XOpenDisplay(display_name);
	if (display == NULL)
		return 1;
	return 0;
}

void x_exit(void)
{
	XCloseDisplay(display);
}

Atom x_get_atom(enum atom_index idx)
{
	if (atom_values[idx] == 0)
		atom_values[idx] = XInternAtom(display, atom_names[idx], False);
	return atom_values[idx];
}

int x_get_property(Window window, Atom type, Atom property, size_t type_size, void **prop_ret, int *nr_ret)
{
	int rc, n;
	char *p;

	rc = get_property_array(window, type, property, &p, &n);
	if (rc)
		return rc;
	*prop_ret = xmalloc(n * type_size);
	memcpy(*prop_ret, p, n * type_size);
	*nr_ret = n;
	XFree(p);
	return 0;
}

int x_get_property_nr(Window window, Atom type, Atom property, size_t type_size, void *prop_ret, int nr)
{
	int rc, n;
	char *p;

	rc = get_property_array(window, type, property, &p, &n);
	if (rc)
		return rc;
	if (n != nr)
		return -2;
	memcpy(prop_ret, p, nr * type_size);
	XFree(p);
	return 0;
}

int x_get_string_property(Window window, Atom property, char **prop_ret)
{
	return get_str_property(window, XA_STRING, property, prop_ret);
}

int x_get_string_array_property(Window window, Atom property, char ***prop_ret, int *nr_ret)
{
	return get_str_array_property(window, XA_STRING, property, prop_ret, nr_ret);
}

int x_get_utf8_string_property(Window window, Atom property, char **prop_ret)
{
	return get_str_property(window, x_get_atom(UTF8_STRING), property, prop_ret);
}

int x_get_utf8_string_array_property(Window window, Atom property, char ***prop_ret, int *nr_ret)
{
	return get_str_array_property(window, x_get_atom(UTF8_STRING), property, prop_ret, nr_ret);
}

int x_set_property(Window window, Atom type, Atom property, int format, const void *prop, int nr)
{
	/* return value not documented. fucking xlib */
	XChangeProperty(display, window, property, type, format, PropModeReplace, prop, nr);
	return 0;
}

int x_is_netwm_compatible_wm_running(void)
{
	Window win;

	if (x_get_window_property_nr(DefaultRootWindow(display), x_get_atom(_NET_SUPPORTING_WM_CHECK), &win, 1))
		return 0;
	return 1;
}

int x_get_wm_name(char **wm_name)
{
	Window win;

	if (x_get_window_property_nr(DefaultRootWindow(display), x_get_atom(_NET_SUPPORTING_WM_CHECK), &win, 1))
		return -1;
	return x_get_utf8_string_property(win, x_get_atom(_NET_WM_NAME), wm_name);
}

int x_get_client_list(int stacking, Window **windows, int *nr_windows)
{
	Atom property;

	if (stacking) {
		property = x_get_atom(_NET_CLIENT_LIST_STACKING);
	} else {
		property = x_get_atom(_NET_CLIENT_LIST);
	}
	return x_get_window_property(DefaultRootWindow(display), property, windows, nr_windows);
}

int x_get_client_list_for_desktop(int stacking, int desktop, Window **windows, int *nr_windows)
{
	Window *all_w, *w;
	int rc, nr, i, j;

	rc = x_get_client_list(stacking, &all_w, &nr);
	if (rc)
		return rc;

	w = xnew(Window, nr);
	j = 0;
	for (i = 0; i < nr; i++) {
		int desk;

		if (x_window_get_desktop(all_w[i], &desk) || desk != desktop)
			continue;
		w[j++] = all_w[i];
	}
	free(all_w);
	w = xrenew(Window, w, j);

	*windows = w;
	*nr_windows = j;
	return 0;
}

int x_set_desktop_layout(int columns, int rows)
{
	unsigned long data[4];

	if (columns < 1 || rows < 1)
		return 1;

	if (client_msg(DefaultRootWindow(display), DefaultRootWindow(display),
			_NET_NUMBER_OF_DESKTOPS, columns * rows, 0, 0, 0, 0))
		return -1;

	data[0] = _NET_WM_ORIENTATION_HORZ;
	data[1] = columns;
	data[2] = rows;
	data[3] = _NET_WM_TOPLEFT;
	if (x_set_cardinal_property(DefaultRootWindow(display), x_get_atom(_NET_DESKTOP_LAYOUT), data, 4))
		return -1;
	return 0;
}

int x_get_desktop_layout(int *columns, int *rows)
{
	unsigned long layout[4];
	unsigned long nr_desks;
	int c, r, properties_set_by_idiot = 0;

	/* this is set by WM */
	if (x_get_cardinal_property_nr(DefaultRootWindow(display), x_get_atom(_NET_NUMBER_OF_DESKTOPS), &nr_desks, 1))
		return -1;
	if (nr_desks == 0)
		return -1;

	/* this is set by pager */
	if (x_get_cardinal_property_nr(DefaultRootWindow(display), x_get_atom(_NET_DESKTOP_LAYOUT), layout, 4)) {
		c = nr_desks;
		r = 1;
		properties_set_by_idiot = 1;
	} else {
		c = layout[1];
		r = layout[2];
	}

	if (c == 0 && r == 0) {
		/* layout is crap! */
		c = nr_desks;
		r = 1;
		properties_set_by_idiot = 1;
	} else if (c == 0) {
		/* columns is derived from nr of desks & rows! */
		c = nr_desks / r;
		if (nr_desks % r)
			c++;
		properties_set_by_idiot = 1;
	} else if (r == 0) {
		/* rows is derived from nr of desks & cols! */
		r = nr_desks / c;
		if (nr_desks % c)
			r++;
		properties_set_by_idiot = 1;
	} else {
		/* layout is ok */
		if (c * r != nr_desks) {
			/* argh */
			properties_set_by_idiot = 1;
		}
	}
	if (properties_set_by_idiot)
		x_set_desktop_layout(c, r);

	*columns = c;
	*rows = r;
	return 0;
}

int x_get_desktop_names(char ***namesp)
{
	int nr;

	return x_get_utf8_string_array_property(DefaultRootWindow(display), x_get_atom(_NET_DESKTOP_NAMES), namesp, &nr);
}

int x_set_showing_desktop(int on)
{
	return client_msg(DefaultRootWindow(display), DefaultRootWindow(display),
			_NET_SHOWING_DESKTOP, on != 0, 0, 0, 0, 0);
}

int x_get_showing_desktop(int *on)
{
	unsigned long state;

	if (x_get_cardinal_property_nr(DefaultRootWindow(display), x_get_atom(_NET_SHOWING_DESKTOP), &state, 1))
		return -1;
	*on = state != 0L;
	return 0;
}

int x_set_current_desktop(int desktop)
{
	return client_msg(DefaultRootWindow(display), DefaultRootWindow(display),
			_NET_CURRENT_DESKTOP, desktop, 0, 0, 0, 0);
}

int x_get_current_desktop(int *desktop)
{
	unsigned long tmp;

	if (x_get_cardinal_property_nr(DefaultRootWindow(display), x_get_atom(_NET_CURRENT_DESKTOP), &tmp, 1))
		return -1;
	*desktop = tmp;
	return 0;
}

int x_set_active_window(Window window, enum source_indication si)
{
	/* FIXME: [1] timestamp, [2] requestor's currently active window */
	return client_msg(DefaultRootWindow(display), window, _NET_ACTIVE_WINDOW, si, 0, 0, 0, 0);
}

int x_get_active_window(Window *window)
{
	int rc;

	rc = x_get_window_property_nr(DefaultRootWindow(display), x_get_atom(_NET_ACTIVE_WINDOW), window, 1);
	if (rc == 0 && *window == 0)
		return -2;
	return rc;
}

int x_get_root_pixmap(Pixmap *pixmap)
{
	return x_get_property_nr(DefaultRootWindow(display), XA_PIXMAP, x_get_atom(_XROOTPMAP_ID), sizeof(Pixmap), pixmap, 1);
}

int x_window_set_desktop(Window window, int desktop)
{
	unsigned long desk;

	if (desktop == -1) {
		desk = 0xffffffff;
	} else {
		desk = desktop;
	}
	return client_msg(DefaultRootWindow(display), window, _NET_WM_DESKTOP, desk, 0, 0, 0, 0);
}

int x_window_get_desktop(Window window, int *desktop)
{
	unsigned long desk;

	if (x_get_cardinal_property_nr(window, x_get_atom(_NET_WM_DESKTOP), &desk, 1))
		return -1;

	if (desk == 0xffffffff) {
		*desktop = -1;
	} else {
		*desktop = desk;
	}
	return 0;
}

/*
 * _NET_WM_NAME:
 *      window title
 *
 * _NET_WM_VISIBLE_NAME:
 *      set by window manager if >1 windows with same title
 *      for example 2 xterms => 'xterm [1]' and 'xterm [2]'
 *
 * _NET_WM_ICON_NAME:
 * _NET_WM_VISIBLE_ICON_NAME:
 *
 * WM_NAME:
 *      same as _NET_WM_NAME which is preferred
 *
 * WM_ICON_NAME:
 *      same as _NET_WM_ICON_NAME which is preferred
 */

int x_window_set_title(Window window, const char *title)
{
	x_set_utf8_string_property(window, x_get_atom(_NET_WM_NAME), title);
	x_set_utf8_string_property(window, x_get_atom(_NET_WM_ICON_NAME), title);

	x_set_string_property(window, x_get_atom(WM_NAME), title);
	x_set_string_property(window, x_get_atom(WM_ICON_NAME), title);
	return 0;
}

int x_window_get_title(Window window, char **title)
{
	char *tmp;
	int rc;

	/* -2 => BadWindow */
	rc = x_get_utf8_string_property(window, x_get_atom(_NET_WM_VISIBLE_NAME), &tmp);
	if (rc == -2)
		return -1;
	if (rc) {
		rc = x_get_utf8_string_property(window, x_get_atom(_NET_WM_NAME), &tmp);
		if (rc == -2)
			return -1;
		if (rc) {
			if (x_get_string_property(window, x_get_atom(WM_NAME), &tmp))
				return -1;
		}
	}
	*title = xstrdup(tmp);
	XFree(tmp);
	return 0;
}

int x_window_set_geometry(Window window, int flags, int x, int y, unsigned int w, unsigned int h)
{
	unsigned int mask = 0;
	XWindowChanges values;
	long user_supplied;
	XSizeHints hints;
	int old_x, old_y, old_w, old_h;

	if (x_window_get_geometry(window, &old_x, &old_y, &old_w, &old_h)) {
		d_print("could not get geometry of window 0x%x\n", (int)window);
		return -1;
	}

	if (XGetWMNormalHints(display, window, &hints, &user_supplied) == False)
		hints.flags = 0;

	if (flags & WidthValue) {
		/* calculate new h if aspect set */
		if (hints.flags & PAspect) {
/* 			d_print("min aspect: %d/%d\n", hints.min_aspect.x, hints.min_aspect.y); */
/* 			d_print("max aspect: %d/%d\n", hints.max_aspect.x, hints.max_aspect.y); */
			h = w * hints.min_aspect.y / hints.min_aspect.x;
		}

		/* set values.width and values.height */
		if (hints.flags & PResizeInc) {
			int w_mod = old_w % hints.width_inc;
			int h_mod = old_h % hints.height_inc;

/* 			d_print("w %d %d %d\n", old_w, hints.width_inc, w_mod); */
/* 			d_print("h %d %d %d\n", old_h, hints.height_inc, h_mod); */
			values.width = w * hints.width_inc + w_mod;
			values.height = h * hints.height_inc + h_mod;
		} else {
			values.width = w;
			values.height = h;
		}

		/* clamp values.width and values.height to min/max w/h */
		if (hints.flags & PMinSize) {
			if (values.width < hints.min_width)
				values.width = hints.min_width;
			if (values.height < hints.min_height)
				values.height = hints.min_height;
		}
		if (hints.flags & PMaxSize) {
			if (values.width > hints.max_width)
				values.width = hints.max_width;
			if (values.height > hints.max_height)
				values.height = hints.max_height;
		}
		mask |= CWWidth;
		mask |= CWHeight;

		/* update old_w and old_h. needed if x or y negative */
		old_w = values.width;
		old_h = values.height;
	}
	if (flags & XValue) {
		if (flags & XNegative) {
			values.x = DisplayWidth(display, DefaultScreen(display)) + x - old_w;
		} else {
			values.x = x;
		}
		if (flags & YNegative) {
			values.y = DisplayHeight(display, DefaultScreen(display)) + y - old_h;
		} else {
			values.y = y;
		}
		mask |= CWX;
		mask |= CWY;
	}

	if (!XConfigureWindow(display, window, mask, &values)) {
		d_print("could not get configure window 0x%x\n", (int)window);
		return -1;
	}
	return 0;
}

int x_window_get_geometry(Window window, int *x, int *y, int *w, int *h)
{
	XWindowAttributes a;
	Window child_ret;

	if (XGetWindowAttributes(display, window, &a) != True) {
		d_print("could not get attributes of window 0x%x\n", (int)window);
		return -1;
	}
	/* a.x and a.y are always 0 */
/*
 * 	if (a.map_state == IsUnmapped) {
 * 		d_print("window 0x%x is not mapped\n", (int)window);
 * 		return -1;
 * 	}
 */
	if (XTranslateCoordinates(display, window, DefaultRootWindow(display), a.x, a.y, x, y, &child_ret) == True) {
		*w = a.width;
		*h = a.height;
	} else {
		d_print("window and root are in different screens!\n");
		return -1;
	}
	return 0;
}

int x_window_set_type(Window window, enum window_type type)
{
	Atom atom;

	if (type < 0 || type > WINDOW_TYPE_NORMAL) {
		d_print("window type out of range\n");
		return -1;
	}
	atom = x_get_atom(_NET_WM_WINDOW_TYPE_DESKTOP + type);
	return x_set_atom_property(window, x_get_atom(_NET_WM_WINDOW_TYPE), &atom, 1);
}

int x_window_get_type(Window window, enum window_type *type)
{
	int i;
	Atom atom;

	if (x_get_atom_property_nr(window, x_get_atom(_NET_WM_WINDOW_TYPE), &atom, 1))
		return -1;

	/* convert the atom to enum */
	*type = -1;
	for (i = _NET_WM_WINDOW_TYPE_DESKTOP; i <= _NET_WM_WINDOW_TYPE_NORMAL; i++) {
		if (atom == x_get_atom(i)) {
			*type = i - _NET_WM_WINDOW_TYPE_DESKTOP;
			break;
		}
	}
	return 0;
}

int x_window_set_strut_partial(Window window, enum strut_type type, int size, int start, int end)
{
	unsigned long data[12] = { 0, };

	switch (type) {
	case STRUT_TYPE_LEFT:
		data[0] = size;
		data[4] = start;
		data[5] = end;
		break;
	case STRUT_TYPE_RIGHT:
		data[1] = size;
		data[6] = start;
		data[7] = end;
		break;
	case STRUT_TYPE_TOP:
		data[2] = size;
		data[8] = start;
		data[9] = end;
		break;
	case STRUT_TYPE_BOTTOM:
		data[3] = size;
		data[10] = start;
		data[11] = end;
		break;
	}
/* 	for (int i = 0; i < 12; i++) */
/* 		d_print("STRUT[%d]: %d\n", i, (int)data[i]); */
	return x_set_cardinal_property(window, x_get_atom(_NET_WM_STRUT_PARTIAL), data, 12);
}

int x_window_set_opacity(Window window, double opacity)
{
	unsigned long val;
	Window top_parent;

	if (x_window_find_top_parent(window, &top_parent))
		return -1;
	val = (unsigned int)(opacity * (double)0xffffffff);
	return x_set_cardinal_property(top_parent, x_get_atom(_NET_WM_WINDOW_OPACITY), &val, 1);
}

int x_window_get_opacity(Window window, double *opacity)
{
	unsigned long val;
	Window top_parent;

	if (x_window_find_top_parent(window, &top_parent))
		return -1;
	if (x_get_cardinal_property_nr(top_parent, x_get_atom(_NET_WM_WINDOW_OPACITY), &val, 1))
		return -1;
	*opacity = (double)val / (double)0xffffffff;
	return 0;
}

int x_window_set_states(Window window, enum state_action action, enum atom_index prop1, enum atom_index prop2)
{
	Atom a1, a2;

	if (prop1 < _NET_WM_STATE_MODAL || prop1 > _OB_WM_STATE_UNDECORATED) {
		d_print("prop1 must be _NET_WM_STATE_*\n");
		return -1;
	}		
	if (prop2 != 0 && (prop2 < _NET_WM_STATE_MODAL || prop2 > _OB_WM_STATE_UNDECORATED)) {
		d_print("prop2 must be _NET_WM_STATE_* or 0\n");
		return -1;
	}		
	a1 = x_get_atom(prop1);
	if (prop2 == 0) {
		a2 = 0;
	} else {
		a2 = x_get_atom(prop2);
	}
	return client_msg(DefaultRootWindow(display), window, _NET_WM_STATE, action, a1, a2, 0, 0);
}

int x_window_get_states(Window window, unsigned int *states)
{
	Atom *atoms;
	int nr_atoms, i, j;

	if (x_get_atom_property(window, x_get_atom(_NET_WM_STATE), &atoms, &nr_atoms))
		return -1;
	*states = 0;
	for (i = 0; i < nr_atoms; i++) {
		Atom a = atoms[i];

		for (j = _NET_WM_STATE_MODAL; j <= _NET_WM_STATE_BELOW; j++) {
			if (a == x_get_atom(j))
				*states |= 1 << (j - _NET_WM_STATE_MODAL);
		}
	}
	free(atoms);
	return 0;
}

/* FIXME: gets only the first icon */
int x_window_get_icon(Window window, int *width, int *height, char **data)
{
	char *tmp;
	int nr;
	unsigned int w, h;

	if (get_property_array(window, XA_CARDINAL, x_get_atom(_NET_WM_ICON), &tmp, &nr))
		return -1;
	if (nr < 2)
		goto err;
	
	w = ((unsigned int *)tmp)[0];
	h = ((unsigned int *)tmp)[1];
	if (nr < 2 + w * h)
		goto err;
	d_print("n = %d, w * h = %d, w = %d, h = %d\n", nr, w * h, w, h);

	*data = xnew(char, w * h * 4);
	memcpy(*data, tmp + 8, w * h * 4);
	*width = w;
	*height = h;
	XFree(tmp);
	return 0;
err:
	XFree(tmp);
	return -1;
}

int x_window_set_aspect(Window window, int x, int y)
{
	XSizeHints hints;

	hints.flags = PAspect;
	hints.min_aspect.x = x;
	hints.min_aspect.y = y;
	hints.max_aspect.x = x;
	hints.max_aspect.y = y;
	XSetWMNormalHints(display, window, &hints);
	return 0;
}

int x_window_close(Window window)
{
	return client_msg(DefaultRootWindow(display), window, _NET_CLOSE_WINDOW,
			0, 0, 0, 0, 0);
}

int x_window_move_resize(Window window,
		unsigned long x_root_button, unsigned long y_root_button,
		unsigned long direction, unsigned long button)
{
	return client_msg(DefaultRootWindow(display), window, _NET_WM_MOVERESIZE,
			x_root_button, y_root_button, direction, button, 0);
}

int x_parse_color(const char *name, unsigned long *color)
{
	XColor ecolor;

	if (!XParseColor(display, DefaultColormap(display, DefaultScreen(display)), name, &ecolor)) {
		d_print("unknown color `%s'\n", name);
		return -1;
	}
	if (!XAllocColor(display, DefaultColormap(display, DefaultScreen(display)), &ecolor)) {
		d_print("unable to allocate color for `%s'\n", name);
		return -1;
	}
	*color = ecolor.pixel;
	return 0;
}

int x_get_window_by_name(const char *name, Window *window)
{
	Window *windows;
	int rc, nr_windows, i;

	rc = x_get_client_list(0, &windows, &nr_windows);
	if (rc)
		return rc;
	for (i = 0; i < nr_windows; i++) {
		char *win_name;

		if (x_window_get_title(windows[i], &win_name))
			continue;
		if (strcmp(win_name, name) == 0) {
			*window = windows[i];
			free(win_name);
			free(windows);
			return 1;
		}
		free(win_name);
	}
	free(windows);
	return 0;
}

int x_window_set_modal(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_MODAL, 0);
}

/* NOTE: this is not same as desktop = -1 */
int x_window_set_sticky(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_STICKY, 0);
}

int x_window_set_maximized_vert(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_MAXIMIZED_VERT, 0);
}

int x_window_set_maximized_horz(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_MAXIMIZED_HORZ, 0);
}

int x_window_set_shaded(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_SHADED, 0);
}

int x_window_set_skip_taskbar(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_SKIP_TASKBAR, 0);
}

int x_window_set_skip_pager(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_SKIP_PAGER, 0);
}

int x_window_set_hidden(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_HIDDEN, 0);
}

int x_window_set_fullscreen(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_FULLSCREEN, 0);
}

int x_window_set_above(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_ABOVE, 0);
}

int x_window_set_below(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_BELOW, 0);
}

int x_window_set_demands_attention(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_DEMANDS_ATTENTION, 0);
}

int x_window_set_maximized(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _NET_WM_STATE_MAXIMIZED_VERT, _NET_WM_STATE_MAXIMIZED_HORZ);
}

int x_window_set_undecorated(Window window, enum state_action action)
{
	return x_window_set_states(window, action, _OB_WM_STATE_UNDECORATED, 0);
}
