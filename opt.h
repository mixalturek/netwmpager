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

#ifndef _OPT_H
#define _OPT_H

struct option {
	const char *name;
	unsigned int has_arg : 1;
};

enum {
	OPT_ERROR_SUCCESS,
	OPT_ERROR_UNRECOGNIZED_OPTION,
	OPT_ERROR_AMBIGUOUS_OPTION,
	OPT_ERROR_MISSING_ARGUMENT,
	OPT_ERROR_UNKNOWN,
	OPT_NR_ERRORS
};

/*
 * *argsp:
 *   points to command line arguments
 *   args[0] is the first argument (not program name)
 *
 * options:
 *   array of struct option. terminated with { NULL, 0 }
 *
 * option_handler:
 *   arguments:
 *     opt - index to options array
 *     arg - argument to the option or NULL
 *   returns 0 on success, -1 on error
 *
 * error_handler:
 *   if NULL then standard error handler is used
 *   arguments:
 *     args - command line arguments (args[0] is where the error is)
 *     error - OPT_ERROR_*
 *
 * returns number of arguments parsed
 */
extern int options_parse(char ***argsp, const struct option *options,
		int (*option_handler)(int opt, const char *arg),
		int (*error_handler)(char **args, int error));

#endif
