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

#include <stdbool.h>

enum vcmd {
	CMD_PROTOCOL_PREAMBLE,
	CMD_VIDEOHUB_DEVICE,
	CMD_INPUT_LABELS,
	CMD_OUTPUT_LABELS,
	CMD_VIDEO_OUTPUT_LOCKS,
	CMD_VIDEO_OUTPUT_ROUTING,
	CMD_PING,
};

bool parse_command(struct videohub_data *d, char *cmd);
int parse_text_buffer(struct videohub_data *data, char *cmd_buffer);

#endif
