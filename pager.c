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

#include <pager.h>
#include <x.h>
#include <xmalloc.h>
#include <debug.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define WM_WAIT 15

#define TITLE_HPAD	3
#define TITLE_VPAD	1
#define POPUP_PAD	5

/* minimum draw size of window. makes moving small windows possible
 * these must be >= 2 (window borders take 2 pixels)
 */
#define WINDOW_MIN_W	3
#define WINDOW_MIN_H	3

#define WINDOW_SHADED_H	12

#define DRAG_THRESHOLD	2

/* ---------------------------------------------------------------------------
 * PRIVATE
 */

struct client_window {
	Window window;
	int x, y, w, h;

	/* -1 = sticky */
	int desk;

	enum window_type type;
	char *name;

	/* WINDOW_STATE_* */
	unsigned int states;

	int icon_w, icon_h;
	char *icon_data;
};

struct pager {
	Window window;
	Window popup_window;
	Pixmap pixmap;
	GC active_win_gc;
	GC inactive_win_gc;
	GC active_desk_gc;
	GC inactive_desk_gc;
	GC win_border_gc;
	GC grid_gc;

	struct client_window *windows;
	int nr_windows;

	Window active_win;
	int active_desk;

	int cols, rows;

	int x, y;
	int w, h;

	/* user set geometry, these are constants
	 *
	 * if gh is 0 then height (h) is calculated and aspect ratio set
	 *
	 * pager geometry is calculated from these when window size changes
	 * or desktop layout changes
	 */
	int gx, gy, gw, gh;
	unsigned int gx_negative : 1;
	unsigned int gy_negative : 1;

	int root_w;
	int root_h;
	int desk_w;
	int desk_h;

	int w_extra;
	int h_extra;

	/* index to windows[] or -1. used to get the title of the window */
	int popup_idx;
	XGlyphInfo popup_extents;

	unsigned int popup_visible : 1;

	unsigned int needs_configure : 1;
	unsigned int needs_update : 1;
	unsigned int needs_update_properties : 1;
	unsigned int needs_update_popup : 1;

	unsigned int show_sticky : 1;
	unsigned int show_window_titles : 1;
	unsigned int show_window_icons : 1;
	unsigned int show_popups : 1;
	unsigned int allow_cover : 1;

	double opacity;

	enum pager_layer layer;

	struct {
		/* index to windows[] or -1. the window we are moving */
		int window_idx;

		/* mouse coordinates relative to the pager window */
		int window_x;
		int window_y;

		/* mouse button or -1 */
		int button;

		/* click coordinates relative to pager */
		int click_x;
		int click_y;

		unsigned int dragging : 1;
	} mouse;

	XftDraw *xft_draw;
	XftColor active_win_font_color;
	XftColor inactive_win_font_color;
	XftColor popup_font_color;
	XftFont *window_font;
	XftFont *popup_font;
};

static GC make_gc(Window window, const char *color)
{
	GC gc;
	XGCValues values;
	unsigned long fg;
/* 	unsigned long bg; */
	
	x_parse_color(color, &fg);
	values.foreground = fg;
/* 	x_parse_color("rgb:ff/00/00", &bg); */
/* 	values.background = bg; */
	values.line_width = 1;
	values.line_style = LineSolid;
	gc = XCreateGC(display, window, GCForeground, &values);
/* 	gc = XCreateGC(display, window, GCForeground | GCBackground, &values); */
/* 	gc = XCreateGC(display, window, GCForeground | GCBackground | GCLineStyle | GCLineWidth, &values); */
	return gc;
}

static void pager_free_windows(struct pager *pager)
{
	int i;

	for (i = 0; i < pager->nr_windows; i++) {
		free(pager->windows[i].name);
		free(pager->windows[i].icon_data);
	}
	free(pager->windows);
	pager->windows = NULL;
	pager->nr_windows = 0;
}

static void pager_calc_x_y(struct pager *pager)
{
	if (pager->gx_negative) {
		pager->x = pager->root_w - pager->w - pager->gx;
	} else {
		pager->x = pager->gx;
	}
	if (pager->gy_negative) {
		pager->y = pager->root_h - pager->h - pager->gy;
	} else {
		pager->y = pager->gy;
	}
}

static void pager_update_strut(struct pager *pager)
{
	if (pager->allow_cover)
		return;

	if (pager->x >= pager->root_w || pager->y >= pager->root_h) {
		d_print("pager is outside of screen (%d,%d %dx%d)\n", pager->x, pager->y, pager->w, pager->h);
		return;
	}

	if (pager->w > pager->h) {
		if (pager->y == 0) {
			x_window_set_strut_partial(pager->window, STRUT_TYPE_TOP, pager->h, pager->x, pager->x + pager->w);
		} else if (pager->y + pager->h == pager->root_h) {
			x_window_set_strut_partial(pager->window, STRUT_TYPE_BOTTOM, pager->h, pager->x, pager->x + pager->w);
		} else {
			d_print("pager not at edge of screen (%d,%d %dx%d)\n", pager->x, pager->y, pager->w, pager->h);
		}
	} else {
		if (pager->x == 0) {
			x_window_set_strut_partial(pager->window, STRUT_TYPE_LEFT, pager->w, pager->y, pager->y + pager->h);
		} else if (pager->x + pager->w == pager->root_w) {
			x_window_set_strut_partial(pager->window, STRUT_TYPE_RIGHT, pager->w, pager->y, pager->y + pager->h);
		} else {
			d_print("pager not at edge of screen (%d,%d %dx%d)\n", pager->x, pager->y, pager->w, pager->h);
		}
	}
}

static void pager_configure(struct pager *pager)
{
	int x, y;

	if (x_window_get_geometry(pager->window, &x, &y, &pager->w, &pager->h)) {
		d_print("x_window_get_geometry failed\n");
		return;
	}
	pager_calc_x_y(pager);
	if (x != pager->x || y != pager->y) {
		d_print("forcing position\n");
		x_window_set_geometry(pager->window, XValue | YValue, pager->x, pager->y, 0, 0);
	}

	pager->needs_configure = 0;
	pager->needs_update = 1;

	pager->desk_w = (pager->w - (pager->cols - 1)) / pager->cols;
	pager->desk_h = (pager->h - (pager->rows - 1)) / pager->rows;
	pager->w_extra = pager->w - pager->cols * pager->desk_w - (pager->cols - 1);
	pager->h_extra = pager->h - pager->rows * pager->desk_h - (pager->rows - 1);
	XFreePixmap(display, pager->pixmap);
	pager->pixmap = XCreatePixmap(display,
			pager->window,
			pager->w,
			pager->h,
			DefaultDepth(display, DefaultScreen(display)));
	XSetWindowBackgroundPixmap(display, pager->window, pager->pixmap);
	pager_update_strut(pager);
}

static void pager_calc_w_h(struct pager *pager)
{
	pager->w = pager->gw;
	pager->h = pager->gh;
	if (!pager->gh)
		pager->h = pager->w * (pager->root_h * pager->rows) / (pager->root_w * pager->cols);
}

static void pager_update_aspect(struct pager *pager)
{
	if (!pager->gh)
		x_window_set_aspect(pager->window,
				pager->root_w * pager->cols,
				pager->root_h * pager->rows);
}

static void update_desktop_count(struct pager *pager)
{
	int rows = pager->rows;
	int cols = pager->cols;

	if (x_get_desktop_layout(&cols, &rows)) {
		d_print("x_get_desktop_layout failed\n");
	}
	if (rows != pager->rows || cols != pager->cols) {
		pager->rows = rows;
		pager->cols = cols;
		pager_update_aspect(pager);

		pager_calc_w_h(pager);
		/* pager_configure sets right values for these */
		pager->w_extra = 0;
		pager->h_extra = 0;

		if (x_window_set_geometry(pager->window, WidthValue | HeightValue, 0, 0, pager->w, pager->h)) {
		}
		XFreePixmap(display, pager->pixmap);
		pager->pixmap = XCreatePixmap(display,
				pager->window,
				pager->w,
				pager->h,
				DefaultDepth(display, DefaultScreen(display)));

		pager->needs_configure = 1;
	}
}

static int get_window_index(struct pager *pager, Window window)
{
	int i;

	for (i = 0; i < pager->nr_windows; i++) {
		struct client_window *win = &pager->windows[i];

		if (win->window == window)
			return i;
	}
	return -1;
}

extern int ignore_bad_window;

static void pager_update_properties(struct pager *pager)
{
	Window move_win = -1, popup_win = -1;
	Window *windows;
	int nr_windows;
	int i, j;

	update_desktop_count(pager);

	/* stacking order */
	if (x_get_client_list(1, &windows, &nr_windows) == -1) {
		fprintf(stderr, "x_get_client_list (stacking order) failed\n");
		return;
	}

	pager->needs_update_properties = 0;
	pager->needs_update = 1;

	if (pager->mouse.window_idx != -1)
		move_win = pager->windows[pager->mouse.window_idx].window;
	if (pager->popup_idx != -1)
		popup_win = pager->windows[pager->popup_idx].window;

	pager_free_windows(pager);

	pager->nr_windows = nr_windows;
	pager->windows = xnew(struct client_window, pager->nr_windows);

	j = 0;
	for (i = 0; i < pager->nr_windows; i++) {
		struct client_window *win = &pager->windows[j];

		win->window = windows[i];

		ignore_bad_window = 1;

		win->type = WINDOW_TYPE_NORMAL;
		if (x_window_get_type(win->window, &win->type)) {
/* 			fprintf(stderr, "could not get window type of window 0x%x\n", (int)windows[i]); */
/* 			continue; */
		}

		win->states = 0;
		if (x_window_get_states(win->window, &win->states)) {
/* 			fprintf(stderr, "could not get states of window 0x%x\n", (int)windows[i]); */
			continue;
		}

		ignore_bad_window = 0;

		if (win->states & WINDOW_STATE_SKIP_PAGER) {
/* 			d_print("skip pager 0x%x\n", (int)windows[i]); */
			continue;
		}
		win->desk = -1;
		if (x_window_get_desktop(win->window, &win->desk)) {
			fprintf(stderr, "could not get desktop of window 0x%x\n", (int)win->window);
			continue;
		}

		if (x_window_get_geometry(win->window, &win->x, &win->y, &win->w, &win->h)) {
			fprintf(stderr, "could not get geometry of window 0x%x\n", (int)win->window);
			continue;
		}
		if (x_window_get_title(win->window, &win->name)) {
			fprintf(stderr, "could not get name of window 0x%x\n", (int)win->window);
			/* continue; */
			win->name = xstrdup("?");
		}
		win->icon_w = -1;
		win->icon_h = -1;
		win->icon_data = NULL;
/* 		d_print("new window 0x%x '%s'\n", (int)win->window, win->name); */

		/* FIXME: breaks sometimes */
/* 		XSelectInput(display, win->window, PropertyChangeMask); */
		j++;
	}
	ignore_bad_window = 0;
	pager->nr_windows = j;
	free(windows);

	if (move_win != -1)
		pager->mouse.window_idx = get_window_index(pager, move_win);
	if (popup_win != -1)
		pager->popup_idx = get_window_index(pager, popup_win);

	x_get_current_desktop(&pager->active_desk);
	x_get_active_window(&pager->active_win);
}

static void do_draw_window(struct pager *pager, struct client_window *window, int px, int py, int pw, int ph)
{
	GC gc;

	if (pager->active_win == window->window) {
		gc = pager->active_win_gc;
	} else {
		gc = pager->inactive_win_gc;
	}

	/* NOTE: XDrawRectangle draws rectangle one pixel larger than XFillRectangle */
	XDrawRectangle(display, pager->pixmap, pager->win_border_gc, px, py, pw - 1, ph - 1);

	px++;
	py++;
	pw -= 2;
	ph -= 2;
	XFillRectangle(display, pager->pixmap, gc, px, py, pw, ph);

	if (pager->show_window_titles) {
		XftColor *color;
		XGlyphInfo extents;
		XRectangle ra;
		int len = strlen(window->name);
		int x, y, w, h;

		ra.x = px + TITLE_HPAD;
		ra.y = py + TITLE_VPAD;
		w = pw - 2 * TITLE_HPAD;
		h = ph - 2 * TITLE_VPAD;

		if (w < 1 || h < 1)
			return;

		ra.width = w;
		ra.height = h;

		XftTextExtentsUtf8(display, pager->window_font, (FcChar8 *)window->name, len, &extents);
/* 		d_print("w = %d, h = %d, x = %d, y = %d, xoff = %d, yoff = %d\n", extents.width, extents.height, extents.x, extents.y, extents.xOff, extents.yOff); */

		x = ra.x + extents.x;
		y = ra.y + extents.y;
		if (extents.width < ra.width)
			x += (ra.width - extents.width) / 2;
		y += (ra.height - extents.height) / 2;

		if (pager->active_win == window->window) {
			color = &pager->active_win_font_color;
		} else {
			color = &pager->inactive_win_font_color;
		}
		XftDrawSetClipRectangles(pager->xft_draw, 0, 0, &ra, 1);
		XftDrawStringUtf8(pager->xft_draw, color,
				pager->window_font, x, y,
				(FcChar8 *)window->name, len);
	}
}

static void draw_window(struct pager *pager, struct client_window *window)
{
	int col, row;
	int x_off, y_off;
	double x_scale, y_scale;
	int px, py, pw, ph;

	x_scale = (double)pager->root_w / (double)pager->desk_w;
	y_scale = (double)pager->root_h / (double)pager->desk_h;

	px = (int)((double)window->x / x_scale);
	py = (int)((double)window->y / y_scale);
	pw = (int)((double)window->w / x_scale);
	if (window->states & WINDOW_STATE_SHADED) {
		ph = (int)((double)WINDOW_SHADED_H / y_scale);
	} else {
		ph = (int)((double)window->h / y_scale);
	}

	if (pw < WINDOW_MIN_W)
		pw = WINDOW_MIN_W;
	if (ph < WINDOW_MIN_H)
		ph = WINDOW_MIN_H;

	XftDrawChange(pager->xft_draw, pager->pixmap);

	if (window->desk == -1) {
		if (!pager->show_sticky)
			return;

		for (row = 0; row < pager->rows; row++) {
			for (col = 0; col < pager->cols; col++) {
				x_off = col * pager->desk_w + col;
				y_off = row * pager->desk_h + row;

				do_draw_window(pager, window, px + x_off, py + y_off, pw, ph);
			}
		}
	} else {
		col = window->desk % pager->cols;
		row = window->desk / pager->cols;

		x_off = col * pager->desk_w + col;
		y_off = row * pager->desk_h + row;

		do_draw_window(pager, window, px + x_off, py + y_off, pw, ph);
	}
}

/* convert mouse coordinates (@x, @y) (which are relative to the pager window)
 * to root window coordinates (@rx, @ry) and desktop value (@desk)
 */
static void pager_coords_to_root(struct pager *pager, int x, int y, int *rx, int *ry, int *desk)
{
	int row, col;

	col = x / (pager->desk_w + 1);
	row = y / (pager->desk_h + 1);

	x -= col * (pager->desk_w + 1);
	y -= row * (pager->desk_h + 1);

	*rx = x * pager->root_w / pager->desk_w;
	*ry = y * pager->root_h / pager->desk_h;
	*desk = row * pager->cols + col;
}

static int get_window_idx(struct pager *pager, int rx, int ry, int desk)
{
	double scale = (double)pager->root_w / (double)pager->desk_w;
	int i;

	for (i = pager->nr_windows - 1; i >= 0; i--) {
		struct client_window *window = &pager->windows[i];
		int w, h, sw, sh;

		if (!(window->desk == desk || (window->desk == -1 && pager->show_sticky)))
			continue;

		if (window->type == WINDOW_TYPE_DESKTOP)
			continue;

		if (window->states & WINDOW_STATE_HIDDEN)
			continue;

		h = window->h;
		w = window->w;
		if (window->states & WINDOW_STATE_SHADED)
			h = WINDOW_SHADED_H;

		sw = (double)w / scale;
		sh = (double)h / scale;
		if (sw < WINDOW_MIN_W)
			w = (double)WINDOW_MIN_W * scale;
		if (sh < WINDOW_MIN_H)
			h = (double)WINDOW_MIN_H * scale;

		if (rx >= window->x && rx < window->x + w &&
		    ry >= window->y && ry < window->y + h)
			return i;
	}
	return -1;
}

static void pager_update(struct pager *pager)
{
	int row, col, x, y, i;
	int showing_desktop = 0;

	pager->needs_update = 0;

	/* desktops */
	for (row = 0; row < pager->rows; row++) {
		y = row * (pager->desk_h + 1);
		for (col = 0; col < pager->cols; col++) {
			x = col * (pager->desk_w + 1);
			if (row * pager->cols + col == pager->active_desk) {
				XFillRectangle(display, pager->pixmap,
						pager->active_desk_gc,
						x, y, pager->desk_w, pager->desk_h);
			} else {
				XFillRectangle(display, pager->pixmap,
						pager->inactive_desk_gc,
						x, y, pager->desk_w, pager->desk_h);
			}
		}
	}

	if (pager->w_extra) {
		XFillRectangle(display, pager->pixmap,
				pager->inactive_desk_gc,
				pager->w - pager->w_extra,
				0,
				pager->w_extra,
				pager->h);
	}

	if (pager->h_extra) {
		XFillRectangle(display, pager->pixmap,
				pager->inactive_desk_gc,
				0,
				pager->h - pager->h_extra,
				pager->w - pager->w_extra,
				pager->h_extra);
	}

	/* grid */
	for (row = 1; row < pager->rows; row++) {
		y = row * (pager->desk_h + 1) - 1;
		XDrawLine(display, pager->pixmap, pager->grid_gc, 0, y, pager->w, y);
	}
	for (col = 1; col < pager->cols; col++) {
		x = col * (pager->desk_w + 1) - 1;
		XDrawLine(display, pager->pixmap, pager->grid_gc, x, 0, x, pager->h);
	}

	if (x_get_showing_desktop(&showing_desktop)) {
	}
	if (!showing_desktop) {
		/* windows */
		for (i = 0; i < pager->nr_windows; i++) {
			switch (pager->windows[i].type) {
			case WINDOW_TYPE_DESKTOP:
			case WINDOW_TYPE_MENU:
				break;
			case WINDOW_TYPE_DOCK:
			case WINDOW_TYPE_TOOLBAR:
			case WINDOW_TYPE_UTILITY:
			case WINDOW_TYPE_SPLASH:
			case WINDOW_TYPE_DIALOG:
			case WINDOW_TYPE_NORMAL:
				if (!(pager->windows[i].states & WINDOW_STATE_HIDDEN))
					draw_window(pager, &pager->windows[i]);
				break;
			}
		}
	}

	XClearWindow(display, pager->window);
/* 	XFlush(display); */
}

static void pager_update_popup(struct pager *pager)
{
	int len;
	int x, y;
	const char *text;
	XRectangle ra;

	pager->needs_update_popup = 0;
	if (pager->popup_idx == -1) {
		return;
	}

	ra.x = 0;
	ra.y = 0;
	ra.width = pager->popup_extents.width + 2 * POPUP_PAD;
	ra.height = pager->popup_extents.height + 2 * POPUP_PAD;

	text = pager->windows[pager->popup_idx].name;
	len = strlen(text);
	XClearWindow(display, pager->popup_window);
	x = POPUP_PAD + pager->popup_extents.x;
	y = POPUP_PAD + pager->popup_extents.y;
	XftDrawChange(pager->xft_draw, pager->popup_window);
	XftDrawSetClipRectangles(pager->xft_draw, 0, 0, &ra, 1);
	XftDrawStringUtf8(pager->xft_draw, &pager->popup_font_color, pager->popup_font,
			x, y, (FcChar8 *)text, len);
}

static int cursor_to_desk(struct pager *pager, int x, int y)
{
	int row, col;

	col = (x - pager->x) / (pager->desk_w + 1);
	row = (y - pager->y) / (pager->desk_h + 1);
	return row * pager->cols + col;
}

/* show popup window below (or above) the window in the pager */
static void popup_show(struct pager *pager, int cx, int cy)
{
	struct client_window *win;
	int x, y, w, h, len, win_row, bw;
	int x_min = 2;
	int y_min = 2;
	int x_max = pager->root_w - 2;
	int y_max = pager->root_h - 2;

	if (!pager->show_popups)
		return;
	
	win = &pager->windows[pager->popup_idx];
	win_row = cursor_to_desk(pager, cx, cy) / pager->cols;
	len = strlen(win->name);

	bw = 1;

	XftTextExtentsUtf8(display, pager->popup_font, (FcChar8 *)win->name, len, &pager->popup_extents);

	w = pager->popup_extents.width + 2 * POPUP_PAD;
	h = pager->popup_extents.y + 2 * POPUP_PAD;

	x_max -= w + bw * 2;
	y_max -= h + bw * 2;

	x = cx - w / 2;
	if (x < x_min)
		x = x_min;
	if (x > x_max)
		x = x_max;

	/* y = 4px below (or above) the window in the pager */
	y = pager->y + win_row * pager->desk_h + win_row +
		(win->y + win->h) * pager->desk_h / pager->root_h + 4;
	if (y + h > y_max)
		y = pager->y + win_row * pager->desk_h + win_row +
			win->y * pager->desk_h / pager->root_h - h - 4;
	if (y < y_min)
		y = y_min;
	XMoveResizeWindow(display, pager->popup_window, x, y, w, h);

	pager_update_popup(pager);
	
	XMapRaised(display, pager->popup_window);
	pager->popup_visible = 1;
}

static void popup_hide(struct pager *pager)
{
	if (pager->popup_visible) {
		XUnmapWindow(display, pager->popup_window);
		pager->popup_visible = 0;
	}
}

/* ---------------------------------------------------------------------------
 * PUBLIC
 */

char *active_win_color = "rgb:99/a6/bf";
char *inactive_win_color = "rgb:f6/f6/f6";
char *active_desk_color = "rgb:65/72/8c";
char *inactive_desk_color = "rgb:ac/ac/ac";
char *win_border_color = "rgb:00/00/00";
char *grid_color = "rgb:ff/ff/ff";

char *active_win_font_color = "rgb:00/00/00";
char *inactive_win_font_color = "rgb:00/00/00";

char *popup_color = "rgb:e6/e6/e6";
char *popup_font_color = "rgb:00/00/00";

struct pager *pager_new(const char *geometry, int cols, int rows)
{
	unsigned long popup_bg;

	struct pager *pager;
	XSetWindowAttributes attrib;
	unsigned long attrib_mask;
	int x, y;
	Visual *visual;
	Colormap cm;
	int gflags, gx, gy;
	unsigned int gw, gh;
	int wm_c = -1, wm_r = -1;

	x_parse_geometry(geometry, &gflags, &gx, &gy, &gw, &gh);
	if (!(gflags & XValue))
		gx = 0;
	if (!(gflags & YValue))
		gy = 0;
	if (!(gflags & WidthValue))
		gw = 100;
	if (!(gflags & HeightValue))
		gh = 0;
	if (gx < 0)
		gx *= -1;
	if (gy < 0)
		gy *= -1;

	/* NetWM compatible window manager must be running */
	while (1) {
		static int count = 0;

		if (x_is_netwm_compatible_wm_running())
			break;
		if (x_get_desktop_layout(&wm_c, &wm_r) == 0) {
			d_print("Got desktop layout (%dx%d), NetWM compatible wm should be running\n", wm_c, wm_r);
			break;
		}
		if (count == 0)
			fprintf(stderr, "NetWM compatible window manager not running. Waiting at most %d seconds.\n", WM_WAIT);
		if (count == WM_WAIT) {
			fprintf(stderr, "Exiting.\n");
			return NULL;
		}
		sleep(1);
		count++;
	}

	if (cols == -1 || rows == -1) {
		/* use desktop layout set by WM */
		if (wm_c == -1 || wm_r == -1) {
			if (x_get_desktop_layout(&wm_c, &wm_r)) {
				fprintf(stderr, "Couldn't get desktop layout.\n");
				return NULL;
			}
		}
		cols = wm_c;
		rows = wm_r;
	} else {
		/* set desktop layout */
		if (cols > 32)
			cols = 32;
		if (rows > 32)
			rows = 32;
		if (x_set_desktop_layout(cols, rows)) {
			fprintf(stderr, "Couldn't set desktop layout (%dx%d).\n", cols, rows);
			return NULL;
		}
	}

	pager = xnew(struct pager, 1);

	if (x_window_get_geometry(DefaultRootWindow(display), &x, &y, &pager->root_w, &pager->root_h)) {
		d_print("x_window_get_geometry for root window failed\n");
		free(pager);
		return NULL;
	}

	pager->cols = cols;
	pager->rows = rows;

	d_print("g: %d,%d %d, %d %d\n", gx, gy, gw, gflags & XNegative, gflags & YNegative);

	pager->gx = gx;
	pager->gy = gy;
	pager->gx_negative = (gflags & XNegative) != 0;
	pager->gy_negative = (gflags & YNegative) != 0;
	pager->gw = gw;
	pager->gh = gh;
	pager_calc_w_h(pager);
	pager_calc_x_y(pager);

	pager->windows = NULL;
	pager->nr_windows = 0;
	pager->active_win = 0;
	pager->active_desk = 0;

	pager->popup_idx = -1;
	pager->popup_visible = 0;

	pager->needs_configure = 1;
	pager->needs_update = 1;
	pager->needs_update_properties = 1;
	pager->needs_update_popup = 0;

	pager->show_sticky = 1;
	pager->show_window_titles = 1;
	pager->show_window_icons = 1;
	pager->show_popups = 1;
	pager->allow_cover = 1;

	pager->opacity = 1.0;
	pager->layer = LAYER_NORMAL;

	pager->mouse.window_idx = -1;
	pager->mouse.button = -1;
	pager->mouse.dragging = 0;
	pager->mouse.click_x = -1;
	pager->mouse.click_y = -1;

	/* main window */
	attrib_mask = CWBackPixmap | CWBorderPixel | CWEventMask;
	attrib.background_pixmap = ParentRelative;
	attrib.border_pixel = 0;
	attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
		PointerMotionMask | EnterWindowMask | LeaveWindowMask |
		ExposureMask;
	pager->window = XCreateWindow(display, DefaultRootWindow(display),
			pager->x, pager->y, pager->w, pager->h,
			0, // border
			CopyFromParent,
			InputOutput,
			CopyFromParent,
			attrib_mask, &attrib);

	/* popup window */
	attrib_mask = CWOverrideRedirect | CWEventMask;
	attrib.override_redirect = True;
	attrib.event_mask = ExposureMask;
	pager->popup_window = XCreateWindow(display, DefaultRootWindow(display),
			0, 0,
			1, 1,
			1, // border
			CopyFromParent,
			InputOutput,
			CopyFromParent,
			attrib_mask, &attrib);

	/* this is freed / recreated soon */
	pager->pixmap = XCreatePixmap(display,
			pager->window,
			8,
			8,
			DefaultDepth(display, DefaultScreen(display)));

	x_window_set_title(pager->window, "netwmpager");
	pager_update_aspect(pager);

	x_set_property(pager->window, XA_STRING,
			XInternAtom(display, "WM_CLASS", False),
			8, "netwmpager\0netwmpager", 22);

	pager->active_win_gc    = make_gc(pager->window, active_win_color);
	pager->inactive_win_gc  = make_gc(pager->window, inactive_win_color);
	pager->active_desk_gc   = make_gc(pager->window, active_desk_color);
	pager->inactive_desk_gc = make_gc(pager->window, inactive_desk_color);
	pager->win_border_gc    = make_gc(pager->window, win_border_color);
	pager->grid_gc          = make_gc(pager->window, grid_color);

	visual = DefaultVisual(display, DefaultScreen(display));
	cm = DefaultColormap(display, DefaultScreen(display));
	pager->xft_draw = XftDrawCreate(display, pager->pixmap, visual, cm);
	XftColorAllocName(display, visual, cm, active_win_font_color, &pager->active_win_font_color);
	XftColorAllocName(display, visual, cm, inactive_win_font_color, &pager->inactive_win_font_color);
	XftColorAllocName(display, visual, cm, popup_font_color, &pager->popup_font_color);

	x_parse_color(popup_color, &popup_bg);
	XSetWindowBackground(display, pager->popup_window, popup_bg);

	pager->window_font = NULL;
	pager->popup_font = NULL;
	pager_set_window_font(pager, "fixed");
	pager_set_popup_font(pager, "fixed");

	XSelectInput(display, DefaultRootWindow(display), PropertyChangeMask | SubstructureNotifyMask);
	return pager;
}

void pager_delete(struct pager *pager)
{
	XFreePixmap(display, pager->pixmap);

	XFreeGC(display, pager->active_win_gc);
	XFreeGC(display, pager->inactive_win_gc);
	XFreeGC(display, pager->active_desk_gc);
	XFreeGC(display, pager->inactive_desk_gc);
	XFreeGC(display, pager->win_border_gc);
	XFreeGC(display, pager->grid_gc);

	XftFontClose(display, pager->window_font);
	XftFontClose(display, pager->popup_font);
	XftColorFree(display, XftDrawVisual(pager->xft_draw), XftDrawColormap(pager->xft_draw), &pager->active_win_font_color);
	XftColorFree(display, XftDrawVisual(pager->xft_draw), XftDrawColormap(pager->xft_draw), &pager->inactive_win_font_color);
	XftColorFree(display, XftDrawVisual(pager->xft_draw), XftDrawColormap(pager->xft_draw), &pager->popup_font_color);
	XftDrawDestroy(pager->xft_draw);

	XDestroyWindow(display, pager->window);

	pager_free_windows(pager);
	free(pager);
}

void pager_expose_event(struct pager *pager, XEvent *event)
{
	if (event->xexpose.window == pager->window) {
		pager->needs_update = 1;
	} else {
		pager->needs_update_popup = 1;
	}
}

void pager_configure_notify(struct pager *pager)
{
	pager->needs_configure = 1;
	pager->needs_update_properties = 1;
}

void pager_property_notify(struct pager *pager)
{
	pager->needs_update_properties = 1;
}

void pager_handle_events(struct pager *pager)
{
	if (pager->needs_configure)
		pager_configure(pager);
	if (pager->needs_update_properties)
		pager_update_properties(pager);
	if (pager->needs_update)
		pager_update(pager);
	if (pager->needs_update_popup)
		pager_update_popup(pager);
}

void pager_set_opacity(struct pager *pager, double opacity)
{
	pager->opacity = opacity;
	x_window_set_opacity(pager->window, pager->opacity);
}

void pager_set_layer(struct pager *pager, enum pager_layer layer)
{
	pager->layer = layer;
}

static int set_font(const char *name, XftFont **font)
{
	XftFont *f;

	f = XftFontOpenName(display, DefaultScreen(display), name);
	if (f == NULL)
		return -1;
	if (*font)
		XftFontClose(display, *font);
	*font = f;
	return 0;
}

int pager_set_window_font(struct pager *pager, const char *name)
{
	if (set_font(name, &pager->window_font))
		return -1;
	return 0;
}

int pager_set_popup_font(struct pager *pager, const char *name)
{
	if (set_font(name, &pager->popup_font))
		return -1;
	return 0;
}

void pager_set_show_sticky(struct pager *pager, int on)
{
	pager->show_sticky = on != 0;
}

void pager_set_show_window_titles(struct pager *pager, int on)
{
	pager->show_window_titles = on != 0;
}

void pager_set_show_window_icons(struct pager *pager, int on)
{
	pager->show_window_icons = on != 0;
}

void pager_set_show_popups(struct pager *pager, int on)
{
	pager->show_popups = on != 0;
}

void pager_set_allow_cover(struct pager *pager, int on)
{
	pager->allow_cover = on != 0;
}

void pager_show(struct pager *pager)
{
	pager_configure(pager);
	pager_update(pager);

	x_window_set_type(pager->window, WINDOW_TYPE_DOCK);
	XMapWindow(display, pager->window);

	/* pager->window must be mapped and WM must be running */

	x_window_set_skip_pager(pager->window, _NET_WM_STATE_ADD);
	x_window_set_skip_taskbar(pager->window, _NET_WM_STATE_ADD);
	switch (pager->layer) {
	case LAYER_BELOW:
		x_window_set_above(pager->window, _NET_WM_STATE_REMOVE);
		x_window_set_below(pager->window, _NET_WM_STATE_ADD);
		break;
	case LAYER_NORMAL:
		break;
	case LAYER_ABOVE:
		x_window_set_below(pager->window, _NET_WM_STATE_REMOVE);
		x_window_set_above(pager->window, _NET_WM_STATE_ADD);
		break;
	}
	x_window_set_desktop(pager->window, -1);
}

void pager_button_press(struct pager *pager, int x, int y, int button)
{
	int rx, ry, desk;

	if (pager->popup_visible)
		popup_hide(pager);
	if (button != 1 && button != 2)
		return;
	if (pager->mouse.button != -1)
		return;

	pager->mouse.button = button;
	pager->mouse.dragging = 0;
	pager->mouse.click_x = x;
	pager->mouse.click_y = y;

	pager_coords_to_root(pager, x, y, &rx, &ry, &desk);

	pager->mouse.window_idx = get_window_idx(pager, rx, ry, desk);
	if (pager->mouse.window_idx != -1) {
		pager->mouse.window_x = rx - pager->windows[pager->mouse.window_idx].x;
		pager->mouse.window_y = ry - pager->windows[pager->mouse.window_idx].y;
	}

}

void pager_button_release(struct pager *pager, int x, int y, int button)
{
	int rx, ry, desk;

	if (pager->mouse.button != button)
		return;

	pager_coords_to_root(pager, x, y, &rx, &ry, &desk);

	if (button == 1) {
		if (pager->mouse.window_idx == -1) {
			x_set_current_desktop(desk);
		} else if (!pager->mouse.dragging) {
			x_set_current_desktop(desk);
			x_set_active_window(pager->windows[pager->mouse.window_idx].window, SOURCE_INDICATION_PAGER);
		}
	} else if (button == 2) {
	}

	pager->mouse.window_idx = -1;
	pager->mouse.button = -1;
	pager->mouse.dragging = 0;
}

void pager_motion(struct pager *pager, int x, int y)
{
	int rx, ry, desk;
	struct client_window *window;

	if (pager->mouse.button == -1) {
		/* show / hide popup */
		int idx;
		int cx, cy;
		Window child_ret;

		if (!XTranslateCoordinates(display, pager->window,
					DefaultRootWindow(display),
					x, y, &cx, &cy, &child_ret)) {
			fprintf(stderr, "XTranslateCoordinates failed\n");
			return;
		}
		pager_coords_to_root(pager, x, y, &rx, &ry, &desk);
		idx = get_window_idx(pager, rx, ry, desk);
		if (pager->popup_visible) {
			if (idx == -1) {
				popup_hide(pager);
			} else if (idx != pager->popup_idx) {
				popup_hide(pager);
				pager->popup_idx = idx;
				popup_show(pager, cx, cy);
			}
		} else if (idx != -1) {
			pager->popup_idx = idx;
			popup_show(pager, cx, cy);
		}
	} else if (pager->mouse.window_idx != -1) {
		if (!pager->mouse.dragging &&
				abs(pager->mouse.click_x - x) < DRAG_THRESHOLD &&
				abs(pager->mouse.click_y - y) < DRAG_THRESHOLD)
			return;

		/* move window */
		pager->mouse.dragging = 1;

		pager_coords_to_root(pager, x, y, &rx, &ry, &desk);
		window = &pager->windows[pager->mouse.window_idx];
		if (desk != window->desk && window->desk != -1) {
			x_window_set_desktop(window->window, desk);
			window->desk = desk;
		}
		if (pager->mouse.button == 1) {
			/* move to other desk (already done :)) */
		} else if (pager->mouse.button == 2) {
			/* exact placement */
			int wx, wy;

			/* can't set geometry of a shaded window */
			if (window->states & WINDOW_STATE_SHADED)
				x_window_set_shaded(window->window, _NET_WM_STATE_REMOVE);

			wx = rx - pager->mouse.window_x;
			wy = ry - pager->mouse.window_y;
			x_window_set_geometry(window->window, XValue | YValue, wx, wy, 0, 0);
			window->x = wx;
			window->y = wy;
		}
	}
}

void pager_enter(struct pager *pager, int x, int y)
{
	pager_motion(pager, x, y);
}

void pager_leave(struct pager *pager)
{
	popup_hide(pager);
}
