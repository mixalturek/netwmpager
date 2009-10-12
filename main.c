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

#include <xmalloc.h>
#include <pager.h>
#include <opt.h>
#include <x.h>
#include <sconf.h>

#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

char *program_name = NULL;

static struct pager *pager;
static int running = 1;

static void ignored(XEvent *event)
{
/* 	printf("ignoring event %d\n", event->type); */
}

/*
 * SubstructureNotifyMask:
 *
 * CirculateNotify, ConfigureNotify, CreateNotify, DestroyNotify,
 * GravityNotify, MapNotify, ReparentNotify, UnmapNotify
 */
static void handle_event(XEvent *event)
{
/* 	printf("event %2d, window 0x%x\n", event->type, (int)event->xany.window); */
	switch (event->type) {
	case ButtonPress:
		pager_button_press(pager, event->xbutton.x, event->xbutton.y, event->xbutton.button);
		break;
	case ButtonRelease:
		pager_button_release(pager, event->xbutton.x, event->xbutton.y, event->xbutton.button);
		break;
	case MotionNotify:
		pager_motion(pager, event->xbutton.x, event->xbutton.y);
		break;
	case EnterNotify:
		pager_enter(pager, event->xcrossing.x, event->xcrossing.y);
		break;
	case LeaveNotify:
		pager_leave(pager);
		break;
	case Expose:
		pager_expose_event(pager, event);
		break;
	case CreateNotify:
		ignored(event);
		break;
	case DestroyNotify:
		ignored(event);
		break;
	case UnmapNotify:
		ignored(event);
		break;
	case MapNotify:
		ignored(event);
		break;
	case ReparentNotify:
		ignored(event);
		break;
	case ConfigureNotify:
		pager_configure_notify(pager);
		break;
	case CirculateNotify:
		ignored(event);
		break;
	case PropertyNotify:
		pager_property_notify(pager);
		break;
	case ClientMessage:
		ignored(event);
		break;
	default:
		printf("unexpected event %2d, window 0x%x\n", event->type, (int)event->xany.window);
	}
}

static void loop(void)
{
	while (running) {
		pager_handle_events(pager);
		do {
			XEvent e;

			XNextEvent(display, &e);
			handle_event(&e);
		} while (XPending(display));
	}
}

int ignore_bad_window = 0;

static int xerror_handler(Display *d, XErrorEvent *e)
{
	char buffer[256];

	XGetErrorText(d, e->error_code, buffer, sizeof(buffer) - 1);
	if (e->error_code != BadWindow || !ignore_bad_window) {
		fprintf(stderr, "%s: error: %s:\n"
				" resource id:  0x%x\n"
				" request code: 0x%x\n"
				" minor code:   0x%x\n"
				" serial:       0x%lx\n",
				program_name,
				buffer,
				(unsigned int)e->resourceid,
				e->request_code,
				e->minor_code,
				(unsigned long)e->serial);
	}
	if (e->error_code != BadWindow)
		exit(1);
	return 0;
}

enum {
	OPT_DISPLAY,
	OPT_HELP,
	OPT_VERSION,
	NUM_OPTIONS
};

static struct option options[NUM_OPTIONS + 1] = {
	{ "display",     1 },
	{ "help",        0 },
	{ "version",     0 },
	{ NULL,          0 }
};

/* -- configuration -- */
static const char *display_name = NULL;
static char *window_font = NULL;
static char *popup_font = NULL;
static char *geometry = NULL;
static double opacity = 1.0;
static int show_popups = 1;
static int show_sticky = 1;
static int show_titles = 1;
static int allow_cover = 1;
static int show_icons = 1;
static int cols = -1;
static int rows = -1;
static enum pager_layer layer = LAYER_NORMAL;

static int option_handler(int opt, const char *arg)
{
	switch (opt) {
	case OPT_DISPLAY:
		display_name = arg;
		break;
	case OPT_HELP:
		printf(
"Usage: %s [OPTION]...\n"
"\n"
"  -display NAME      X server to connect to\n"
"  -help              display this help and exit\n"
"  -version           output version information and exit\n"
"\n"
"Fonts:\n"
"  Example: Verdana:size=7\n"
"  Run `fc-list' to see available fonts.\n"
"\n"
"Mouse buttons:\n"
"  1: change desktop, activate window, move window to other desktop\n"
"  2: move window\n"
"\n"
"Config file:    ~/.config/netwmpager/config\n"
"Example config: " DATADIR "/netwmpager/config-example\n"
"\n"
"Report bugs to <tihirvon@gmail.com>.\n" , program_name);
		exit(0);
	case OPT_VERSION:
		printf("netwmpager " VERSION "\nCopyright 2004-2005 Timo Hirvonen\n");
		exit(0);
	}
	return 0;
}

static void load_config(void)
{
	char *str;

	sconf_load();

	if (sconf_get_int_option("cols", &cols) && (cols < 1 && cols != -1)) {
		fprintf(stderr, "%s: cols must be positive integer or -1\n", program_name);
		cols = 1;
	}
	if (sconf_get_int_option("rows", &rows) && (rows < 1 && rows != -1)) {
		fprintf(stderr, "%s: rows must be positive integer or -1\n", program_name);
		rows = 1;
	}
	if (sconf_get_flt_option("opacity", &opacity) && (opacity < 0.0 || opacity > 1.0)) {
		fprintf(stderr, "%s: opacity must be between 0.0 and 1.0\n", program_name);
		opacity = 1.0;
	}
	sconf_get_str_option("geometry", &geometry);
	sconf_get_str_option("popup_font", &popup_font);
	sconf_get_str_option("window_font", &window_font);
	sconf_get_bool_option("show_popups", &show_popups);
	sconf_get_bool_option("show_sticky", &show_sticky);
	sconf_get_bool_option("show_titles", &show_titles);
	sconf_get_bool_option("allow_cover", &allow_cover);
	sconf_get_bool_option("show_icons", &show_icons);

	if (sconf_get_str_option("layer", &str)) {
		if (strcmp(str, "below") == 0) {
			layer = LAYER_BELOW;
		} else if (strcmp(str, "normal") == 0) {
			layer = LAYER_NORMAL;
		} else if (strcmp(str, "above") == 0) {
			layer = LAYER_ABOVE;
		} else {
			fprintf(stderr, "%s: layer must be \"below\", \"normal\" or \"above\"\n", program_name);
		}
		free(str);
	}

	sconf_get_str_option("active_win_color", &active_win_color);
	sconf_get_str_option("inactive_win_color", &inactive_win_color);
	sconf_get_str_option("active_desk_color", &active_desk_color);
	sconf_get_str_option("inactive_desk_color", &inactive_desk_color);
	sconf_get_str_option("win_border_color", &win_border_color);
	sconf_get_str_option("grid_color", &grid_color);

	sconf_get_str_option("active_win_font_color", &active_win_font_color);
	sconf_get_str_option("inactive_win_font_color", &inactive_win_font_color);

	sconf_get_str_option("popup_color", &popup_color);
	sconf_get_str_option("popup_font_color", &popup_font_color);

	sconf_free();
}

int main(int argc, char *argv[])
{
	char **args = argv + 1;
	int nr_args = argc - 1;

	program_name = argv[0];
	load_config();
	nr_args -= options_parse(&args, options, option_handler, NULL);
	if (*args) {
		fprintf(stderr, "%s: too many arguments\n", argv[0]);
		fprintf(stderr, "Try `%s -help' for more information.\n", argv[0]);
		return 1;
	}

	if (x_init(display_name)) {
		fprintf(stderr, "%s: unable to open display %s\n", argv[0],
				display_name ? display_name : "");
		return 1;
	}
	XSetErrorHandler(xerror_handler);

	pager = pager_new(geometry, cols, rows);
	if (pager == NULL) {
		x_exit();
		return 1;
	}

	/* window title must be set before this! */
	XSynchronize(display, True);

	pager_set_layer(pager, layer);
	pager_set_show_sticky(pager, show_sticky);
	pager_set_show_window_titles(pager, show_titles);
	pager_set_show_window_icons(pager, show_icons);
	pager_set_show_popups(pager, show_popups);
	pager_set_allow_cover(pager, allow_cover);
	if (popup_font) {
		if (pager_set_popup_font(pager, popup_font))
			fprintf(stderr, "%s: could not set font '%s'\n",
					argv[0], popup_font);
	}
	if (window_font) {
		if (pager_set_window_font(pager, window_font))
			fprintf(stderr, "%s: could not set font '%s'\n",
					argv[0], window_font);
	}

	pager_show(pager);
	pager_set_opacity(pager, opacity);

	loop();

	pager_delete(pager);
	x_exit();
	return 0;
}
