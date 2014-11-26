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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "util.h"

void die(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, fmt, args);
	if (fmt[strlen(fmt) - 1] != '\n')
		fprintf(stderr, "\n");
	va_end(args);
	exit(EXIT_FAILURE);
}

void *xmalloc(size_t size) {
	void *ret = malloc(size);
	if (!ret)
		die("Can't alloc %ld bytes\n", (unsigned long)size);
	return ret;
}

void *xzalloc(size_t size) {
	void *ret = xmalloc(size);
	memset(ret, 0, size);
	return ret;
}

void *xcalloc(size_t nmemb, size_t size) {
	return xzalloc(nmemb * size);
}

void *xrealloc(void *ptr, size_t size) {
	void *ret = realloc(ptr, size);
	if (!ret)
		die("Can't realloc %ld bytes\n", (unsigned long)size);
	return ret;
}

char *xstrdup(const char *s) {
	char *ret;
	if (!s)
		return NULL;
	ret = strdup(s);
	if (!ret)
		die("Can't strdup %lu bytes\n", (unsigned long)strlen(s) + 1);
	return ret;
}

char *xstrndup(const char *s, size_t n) {
	char *ret;
	if (!s)
		return NULL;
	ret = strndup(s, n);
	if (!ret)
		die("Can't strndup %lu bytes\n", (unsigned long)n + 1);
	return ret;
}

bool streq(const char *s1, const char *s2) {
	if(!s1 && s2) { return 0; }
	if(s1 && !s2) { return 0; }
	if(!s1 && !s2) { return 1; }
	return strcmp(s1, s2) == 0;
}
