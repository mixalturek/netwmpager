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

#include <opt.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

extern char *program_name;

int options_parse(char ***argsp, const struct option *options,
		int (*option_handler)(int opt, const char *arg),
		int (*error_handler)(char **args, int error))
{
	char **args = *argsp;
	int error, i;

	for (i = 0; args[i]; i++) {
		const char *s = args[i];
		const char *arg = NULL;
		int j, idx, num, len, rc;

		if (s[0] != '-') {
			/* no more options */
			break;
		}
		if (s[1] == 0) {
			/* - */
			/* no more options */
			break;
		}
		if (s[1] == '-' && s[2] == 0) {
			/* skip '--' */
			/* no more options */
			i++;
			break;
		}

		/* find option */
		idx = -1;
		num = 0;
		len = strlen(s + 1);
		for (j = 0; options[j].name; j++) {
			if (strncmp(s + 1, options[j].name, len) == 0) {
				/* option found */
				idx = j;
				num++;
				if (options[j].name[len] == 0) {
					/* exact match, ignore other matches */
					num = 1;
					break;
				}
			}
		}
		if (num > 1) {
			error = OPT_ERROR_AMBIGUOUS_OPTION;
			goto __error;
		}
		if (num == 0) {
			error = OPT_ERROR_UNRECOGNIZED_OPTION;
			goto __error;
		}

		/* get option argument if any */
		if (options[idx].has_arg) {
			arg = args[i + 1];
			if (arg == NULL) {
				error = OPT_ERROR_MISSING_ARGUMENT;
				goto __error;
			}
		}

		rc = option_handler(idx, arg);
		if (rc) {
			/* argument was invalid or other error */
			error = OPT_ERROR_UNKNOWN;
			goto __error;
		}
		if (options[idx].has_arg) {
			/* skip the argument */
			i++;
		}
	}
	*argsp = args + i;
	return i;
__error:
	if (error_handler)
		return error_handler(args + i, error);

	/* standard error handling */
	switch (error) {
	case OPT_ERROR_UNRECOGNIZED_OPTION:
		fprintf(stderr, "%s: unrecognized option `%s'\n",
				program_name, args[i]);
		break;
	case OPT_ERROR_AMBIGUOUS_OPTION:
		fprintf(stderr, "%s: option `%s' is ambiguous\n",
				program_name, args[i]);
		break;
	case OPT_ERROR_MISSING_ARGUMENT:
		fprintf(stderr, "%s: option `%s' requires an argument\n",
				program_name, args[i]);
		break;
	case OPT_ERROR_UNKNOWN:
		/* option_handler should already have printed an error message */
		break;
	}
	fprintf(stderr, "Try `%s -help' for more information.\n",
			program_name);
	exit(EXIT_FAILURE);
}
