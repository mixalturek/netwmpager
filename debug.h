/* 
 * Copyright 2004-2005 Timo Hirvonen
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#if DEBUG > 0

#define d_print(...) fprintf(stderr, __VA_ARGS__)

#else

#define d_print(...) do { } while (0)

#endif

#endif
