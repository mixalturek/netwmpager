/* 
 * Copyright 2004-2005 Timo Hirvonen
 */

#include <sconf.h>
#include <list.h>
#include <xmalloc.h>
#include <file.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

extern char *program_name;

struct sconf_option {
	struct list_head node;
	char *name;
	enum { CO_STR, CO_INT, CO_FLT, CO_BOOL } type;
	union {
		char *str_val;
		int int_val;
		double flt_val;
		int bool_val;
	};
};

static LIST_HEAD(head);
static char config_filename[256];
static int line_nr = 0;

static void __NORETURN die(const char *format, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", program_name);
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
	exit(1);
}

static void option_free(struct sconf_option *opt)
{
	free(opt->name);
	if (opt->type == CO_STR)
		free(opt->str_val);
	free(opt);
}

static void add_option(struct sconf_option *new)
{
	struct list_head *item;

	list_for_each(item, &head) {
		struct sconf_option *opt;

		opt = list_entry(item, struct sconf_option, node);
		if (strcmp(opt->name, new->name) == 0) {
			list_del(&opt->node);
			option_free(opt);
			break;
		}
	}
	list_add_tail(&new->node, &head);
}

static int handle_line(void *data, const char *line)
{
	struct sconf_option *opt;
	const char *ns;
	char *end;
	double d;

	line_nr++;
	while (isspace(*line))
		line++;

	if (*line == 0 || *line == '#')
		return 0;

	ns = line;
	while (1) {
		int ch = *line;
		if (!isalnum(ch) && ch != '_' && ch != '.')
			break;
		line++;
	}

	if (ns == line)
		goto syntax;

	opt = xnew(struct sconf_option, 1);
	opt->name = xstrndup(ns, line - ns);

	while (isspace(*line))
		line++;
	if (*line != '=')
		goto syntax;
	line++;

	while (isspace(*line))
		line++;

	d = strtod(line, &end);
	if (*line != 0 && end != line) {
		int i = (int)d;

		if (d != (double)i) {
			opt->type = CO_FLT;
			opt->flt_val = d;
		} else {
			opt->type = CO_INT;
			opt->int_val = i;
		}
		line = end;
	} else if (*line == '"') {
		const char *pos;
		char *str;
		int i = 0;

		line++;
		pos = line;

		str = xnew(char, strlen(line));
		while (1) {
			int ch = *pos++;

			if (ch == '"')
				break;
			if (ch == '\\')
				ch = *pos++;
			if (ch == 0) {
				free(str);
				goto syntax;
			}
			str[i++] = ch;
		}
		str[i] = 0;
		line = pos;

		opt->type = CO_STR;
		opt->str_val = str;
	} else {
		int val;

		if (strncmp(line, "true", 4) == 0) {
			val = 1;
			line += 4;
		} else if (strncmp(line, "false", 5) == 0) {
			val = 0;
			line += 5;
		} else {
			goto syntax;
		}

		opt->type = CO_BOOL;
		opt->bool_val = val;
	}

	while (isspace(*line))
		line++;

	if (*line != 0) {
		option_free(opt);
		goto syntax;
	}
	add_option(opt);
	return 0;
syntax:
	die("syntax error in file `%s` on line %d\n", config_filename, line_nr);
}

static void buffer_for_each_line(const char *buf, int size,
		int (*cb)(void *data, const char *line),
		void *data)
{
	char *line = NULL;
	int line_size = 0, pos = 0;

	while (pos < size) {
		int end, len;

		end = pos;
		while (end < size && buf[end] != '\n')
			end++;

		len = end - pos;
		if (end > pos && buf[end - 1] == '\r')
			len--;

		if (len >= line_size) {
			line_size = len + 1;
			line = xrenew(char, line, line_size);
		}
		memcpy(line, buf + pos, len);
		line[len] = 0;
		pos = end + 1;

		if (cb(data, line))
			break;
	}
	free(line);
}

static int file_for_each_line(const char *filename,
		int (*cb)(void *data, const char *line),
		void *data)
{
	struct stat st;
	char *buf;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return -1;
	if (fstat(fd, &st) == -1) {
		close(fd);
		return -1;
	}
	buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	buffer_for_each_line(buf, st.st_size, cb, data);
	munmap(buf, st.st_size);
	return 0;
}

void sconf_load(void)
{
	char *xdg_config_home;

	xdg_config_home = getenv("XDG_CONFIG_HOME");
	if (xdg_config_home == NULL || xdg_config_home[0] == 0) {
		char *home_dir;

		home_dir = getenv("HOME");
		if (home_dir == NULL)
			die("environment variable HOME not set\n");
		snprintf(config_filename, sizeof(config_filename), "%s/.config/netwmpager/config", home_dir);
	} else {
		snprintf(config_filename, sizeof(config_filename), "%s/netwmpager/config", xdg_config_home);
	}

	if (file_for_each_line(config_filename, handle_line, NULL))
		die("error loading config file `%s': %s", config_filename, strerror(errno));
}

void sconf_free(void)
{
	struct list_head *item, *next;
	struct sconf_option *opt;

	item = head.next;
	while (item != &head) {
		opt = container_of(item, struct sconf_option, node);
		next = item->next;

		if (opt->type == CO_STR)
			free(opt->str_val);
		free(opt);

		item = next;
	}
}

static struct sconf_option *find_opt(const char *name, int type)
{
	struct sconf_option *opt;

	list_for_each_entry(opt, &head, node) {
		if (strcmp(opt->name, name))
			continue;

		if (type == CO_FLT && opt->type == CO_INT) {
			int val = opt->int_val;

			opt->type = CO_FLT;
			opt->flt_val = (double)val;
		}
		if (opt->type != type)
			break;
		return opt;
	}
	return NULL;
}

int sconf_get_str_option(const char *name, char **value)
{
	struct sconf_option *opt = find_opt(name, CO_STR);
	if (opt == NULL)
		return 0;
	*value = xstrdup(opt->str_val);
	return 1;
}

int sconf_get_int_option(const char *name, int *value)
{
	struct sconf_option *opt = find_opt(name, CO_INT);
	if (opt == NULL)
		return 0;
	*value = opt->int_val;
	return 1;
}

int sconf_get_flt_option(const char *name, double *value)
{
	struct sconf_option *opt = find_opt(name, CO_FLT);
	if (opt == NULL)
		return 0;
	*value = opt->flt_val;
	return 1;
}

int sconf_get_bool_option(const char *name, int *value)
{
	struct sconf_option *opt = find_opt(name, CO_BOOL);
	if (opt == NULL)
		return 0;
	*value = opt->bool_val;
	return 1;
}
