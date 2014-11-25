/*
 * === Utility functions ===
 *
 * Blackmagic Design Videohub control application
 * Copyright (C) 2014 Unix Solutions Ltd.
 * Written by Georgi Chorbadzhiyski
 *
 * Released under MIT license.
 * See LICENSE-MIT.txt for license terms.
 *
 */

#ifndef _UTIL_H
#define _UTIL_H

#include <stdbool.h>
#include <inttypes.h>
#include <sys/time.h>

void die(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
void *xmalloc(size_t size);
void *xzalloc(size_t size);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);

bool streq(const char *s1, const char *s2);

#endif
