/*
 * === Commands processing ===
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
	CMD_VIDEO_INPUT_STATUS,
	CMD_VIDEO_OUTPUT_STATUS,
	CMD_PING,
	CMD_ACK,
	CMD_NAK,
};

bool parse_command(struct videohub_data *d, char *cmd);
int parse_text_buffer(struct videohub_data *data, char *cmd_buffer);

struct vcmd_entry {
	enum vcmd		cmd;
	char			*param1;
	char			*param2;
	unsigned int	port_no1;
	unsigned int	port_no2;
	bool			do_lock;
	enum port_lock	lock;
};

void prepare_cmd_entry(struct videohub_data *d, struct vcmd_entry *e);
void format_cmd_text(struct vcmd_entry *e, char *buf, unsigned int bufsz);

void show_cmd(struct videohub_data *d, struct vcmd_entry *e);

#endif
