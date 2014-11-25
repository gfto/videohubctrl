/*
 * === Network functions ===
 *
 * Blackmagic Design Videohub control application
 * Copyright (C) 2014 Unix Solutions Ltd.
 * Written by Georgi Chorbadzhiyski
 *
 * Released under MIT license.
 * See LICENSE-MIT.txt for license terms.
 *
 */

#ifndef _NET_H
#define _NET_H

int connect_client(int socktype, const char *hostname, const char *service);

#endif
