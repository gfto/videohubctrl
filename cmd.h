/*
 * === Command parser ===
 *
 * Blackmagic Design Videohub control application
 * Copyright (C) 2014 Unix Solutions Ltd.
 * Written by Georgi Chorbadzhiyski
 *
 * Released under MIT license.
 * See LICENSE-MIT.txt for license terms.
 *
 */

#ifndef CMD_H
#define CMD_H

bool parse_command(struct videohub_data *d, char *cmd);

#endif
