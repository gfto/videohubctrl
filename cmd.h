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

#define NUM_COMMANDS 19

enum vcmd {
	CMD_PROTOCOL_PREAMBLE,
	CMD_VIDEOHUB_DEVICE,
	CMD_INPUT_LABELS,
	CMD_OUTPUT_LABELS,
	CMD_VIDEO_OUTPUT_LOCKS,
	CMD_VIDEO_OUTPUT_ROUTING,
	CMD_VIDEO_INPUT_STATUS,
	CMD_VIDEO_OUTPUT_STATUS,
	CMD_MONITORING_OUTPUT_LABELS,
	CMD_MONITORING_OUTPUT_LOCKS,
	CMD_MONITORING_OUTPUT_ROUTING,
	CMD_SERIAL_PORT_LABELS,
	CMD_SERIAL_PORT_LOCKS,
	CMD_SERIAL_PORT_ROUTING,
	CMD_SERIAL_PORT_STATUS,
	CMD_SERIAL_PORT_DIRECTIONS,
	CMD_PING,
	CMD_ACK,
	CMD_NAK = (NUM_COMMANDS - 1),
};

enum cmd_flags {
	PARSE_NONE,   /* The result if this command needs no parsing */
	PARSE_CUSTOM, /* Use custom parser for this command */
	PARSE_LABEL,  /* Parse [port_num] [port_text] */
	PARSE_STATUS, /* Parse [port_num] [port_status] */
	PARSE_ROUTE,  /* Parse [port_num] [dest_port] */
	PARSE_LOCK,   /* Parse [port_num] [dest_slot] */
	PARSE_DIR,    /* Parse [port_num] [port_direction]  - for serial ports */
};

struct videohub_commands {
	enum vcmd		cmd;
	enum cmd_flags	type;
	size_t			ports1;
	size_t			ports2;
	const char		*port_id1;
	const char		*port_id2;
	const char		*opt_prefix;
	bool			allow_disconnect;
};

extern struct videohub_commands videohub_commands[NUM_COMMANDS];

bool parse_command(struct videohub_data *d, char *cmd);
int parse_text_buffer(struct videohub_data *data, char *cmd_buffer);

struct vcmd_entry {
	struct videohub_commands *cmd;
	char			*param1;
	char			*param2;
	unsigned int	port_no1;
	unsigned int	port_no2;
	bool			do_lock;
	enum port_lock	lock;
	enum serial_dir	direction;
	bool			clear_port;
};

void prepare_cmd_entry(struct videohub_data *d, struct vcmd_entry *e);
void format_cmd_text(struct vcmd_entry *e, char *buf, unsigned int bufsz);

void show_cmd(struct videohub_data *d, struct vcmd_entry *e);

#endif
