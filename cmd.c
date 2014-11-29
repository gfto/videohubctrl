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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
#include "cmd.h"
#include "util.h"

enum cmd_flags {
	PARSE_NONE      = (1 << 0), /* The result if this command needs no parsing */
	PARSE_CUSTOM    = (1 << 1), /* Use custom parser for this command */
	PARSE_SLOT_TXT  = (1 << 2), /* Parse [slot_num] [slot_text] */
	PARSE_SLOT_DEST = (1 << 3), /* Parse [slot_num] [dest_slot] */
};

static struct videohub_commands {
	enum vcmd	cmd;
	const char	*txt;
	unsigned int	flags;
} videohub_commands[] = {
	{ CMD_PROTOCOL_PREAMBLE,    "PROTOCOL PREAMBLE",    PARSE_CUSTOM },
	{ CMD_VIDEOHUB_DEVICE,      "VIDEOHUB DEVICE",      PARSE_CUSTOM },
	{ CMD_INPUT_LABELS,         "INPUT LABELS",         PARSE_SLOT_TXT },
	{ CMD_OUTPUT_LABELS,        "OUTPUT LABELS",        PARSE_SLOT_TXT },
	{ CMD_VIDEO_OUTPUT_LOCKS,   "VIDEO OUTPUT LOCKS",   PARSE_SLOT_TXT },
	{ CMD_VIDEO_OUTPUT_ROUTING, "VIDEO OUTPUT ROUTING", PARSE_SLOT_DEST },
	{ CMD_VIDEO_INPUT_STATUS,   "VIDEO INPUT STATUS",   PARSE_SLOT_TXT },
	{ CMD_VIDEO_OUTPUT_STATUS,  "VIDEO OUTPUT STATUS",  PARSE_SLOT_TXT },
	{ CMD_PING,                 "PING",                 PARSE_NONE },
	{ CMD_ACK,                  "ACK",                  PARSE_NONE },
	{ CMD_NAK,                  "NAK",                  PARSE_NONE },
};

static const char *get_cmd_text(enum vcmd cmd) {
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(videohub_commands); i++) {
		if (videohub_commands[i].cmd == cmd)
			return videohub_commands[i].txt;
	}
	return "";
}

static char *parse_text(char *line, char *cmd) {
	char *parsed_text = strstr(line, cmd);
	if (parsed_text == line) {
		return parsed_text + strlen(cmd);
	}
	return NULL;
}

bool parse_command(struct videohub_data *data, char *cmd) {
	unsigned int i;
	bool ret = true;
	if (!strlen(cmd))
		return false;
	struct videohub_commands *v = NULL;
	for (i = 0; i < ARRAY_SIZE(videohub_commands); i++) {
		if (!videohub_commands[i].txt)
			continue;
		if (strstr(cmd, videohub_commands[i].txt) == cmd) {
			v = &videohub_commands[i];
			break;
		}
	}

	if (!v) {
		q("WARNING: Videohub sent unknown command!\n");
		q("         Please report this command to author's email: georgi@unixsol.org\n");
		q("         You may use -q or --quiet to suppress the message.\n");
		q("---------8<-----------8<----------- cut here ---------8<------------8<---------\n");
		q("%s\n", cmd);
		q("---------8<-----------8<----------- cut here ---------8<------------8<---------\n");
		return false;
	}

	d("debug: Got '%s' command.\n", v->txt);
	if (debug > 1)
		d("----\n%s\n----\n", cmd);

	char *p, *cmd_data = xstrdup( cmd + strlen(v->txt) + 2 ); // +2 to compensate for :\n at the end of the command
	// Split line by line
	char *line, *saveptr = NULL;
	for(i = 0, line = strtok_r(cmd_data, "\n", &saveptr); line; line = strtok_r(NULL, "\n", &saveptr), i++) {

		// Parse command data response looking like that: "[slot_pos] [slot_data]"
		char *slot_data = NULL;
		unsigned int slot_pos = 0, dest_pos = 0;

		if (v->flags & (PARSE_SLOT_TXT | PARSE_SLOT_DEST)) {
			slot_data = strchr(line, ' ');
			if (!slot_data)
				continue;
			slot_data[0] = '\0'; // Separate slot_pos from slot_data
			slot_data++;
			slot_pos = strtoul(line, NULL, 10);
			if (slot_pos + 1 > data->device.num_video_outputs) {
				q("WARNING: %s - invalid slot %u\n", v->txt, slot_pos);
				continue;
			}
		}

		if (v->flags & PARSE_SLOT_DEST) {
			dest_pos = strtoul(slot_data, NULL, 10);
			if (dest_pos + 1 > data->device.num_video_inputs) {
				q("WARNING: %s - invalid dest %u\n", v->txt, dest_pos);
				continue;
			}
		}

		// Parse commands
		switch (v->cmd) {
		case CMD_PROTOCOL_PREAMBLE:
			if ((p = parse_text(line, "Version: ")))
				snprintf(data->device.protocol_ver, sizeof(data->device.protocol_ver), "%s", p);
			break;

		case CMD_VIDEOHUB_DEVICE:
			if ((p = parse_text(line, "Device present: "))) {
				data->device.dev_present = streq(p, "true");
				data->device.needs_fw_update = streq(p, "needs_update");
			}

			if ((p = parse_text(line, "Model name: ")))
				snprintf(data->device.model_name, sizeof(data->device.model_name), "%s", p);

			if ((p = parse_text(line, "Unique ID: ")))
				snprintf(data->device.unique_id, sizeof(data->device.unique_id) , "%s", p);

			if ((p = parse_text(line, "Video inputs: ")))
				data->device.num_video_inputs = strtoul(p, NULL, 10);

			if ((p = parse_text(line, "Video processing units: ")))
				data->device.num_video_processing_units = strtoul(p, NULL, 10);

			if ((p = parse_text(line, "Video outputs: ")))
				data->device.num_video_outputs = strtoul(p, NULL, 10);

			if ((p = parse_text(line, "Video monitoring output: ")))
				data->device.num_video_monitoring_outputs = strtoul(p, NULL, 10);

			if ((p = parse_text(line, "Serial ports: ")))
				data->device.num_serial_ports = strtoul(p, NULL, 10);
			break;

		case CMD_INPUT_LABELS:
			snprintf(data->inputs[slot_pos].name, sizeof(data->inputs[slot_pos].name), "%s", slot_data);
			break;

		case CMD_OUTPUT_LABELS:
			snprintf(data->outputs[slot_pos].name, sizeof(data->outputs[slot_pos].name), "%s", slot_data);
			break;

		case CMD_VIDEO_INPUT_STATUS:
			snprintf(data->inputs[slot_pos].status, sizeof(data->inputs[slot_pos].status), "%s", slot_data);
			break;

		case CMD_VIDEO_OUTPUT_STATUS:
			snprintf(data->outputs[slot_pos].status, sizeof(data->outputs[slot_pos].status), "%s", slot_data);
			break;

		case CMD_VIDEO_OUTPUT_LOCKS:
			switch (slot_data[0]) {
			case 'O': data->outputs[slot_pos].lock = PORT_LOCKED; break;
			case 'L': data->outputs[slot_pos].lock = PORT_LOCKED_OTHER; break;
			default : data->outputs[slot_pos].lock = PORT_UNLOCKED; break;
			}
			break;

		case CMD_VIDEO_OUTPUT_ROUTING:
			data->outputs[slot_pos].routed_to = dest_pos;

		case CMD_PING:
		case CMD_ACK:
			// Do nothing
			break;
		case CMD_NAK:
			ret = false;
			break;
		}
	}
	free(cmd_data);
	return ret;
}

int parse_text_buffer(struct videohub_data *data, char *cmd_buffer) {
	// The buffer contains only one command, no splitting is needed
	if (!strstr(cmd_buffer, "\n\n"))
		return parse_command(data, cmd_buffer);
	// Split commands and parse them one by one
	int ok_commands = 0;
	char *buf_copy = xstrdup(cmd_buffer);
	char *newcmd, *cmd = buf_copy;
	while(1) {
		newcmd = strstr(cmd, "\n\n"); // Find next command
		if (!newcmd) {
			if (parse_command(data, cmd)) // Parse current command
				ok_commands++;
			break;
		}
		newcmd[0] = '\0'; // Terminate previous command
		if (parse_command(data, cmd)) // Parse previous command
			ok_commands++;
		cmd = newcmd + 2; // Advance cmd to the next command
	}
	free(buf_copy);
	return ok_commands;
}

// Try to find input/output with certain name, return 0 on not found, pos + 1 is found
static int search_video_output_name(struct videohub_data *d, char *name) {
	unsigned int i;
	for(i = 0; i < d->device.num_video_outputs; i++) {
		if (streq(name, d->outputs[i].name)) {
			return i + 1;
		}
	}
	return 0;
}

static int search_video_input_name(struct videohub_data *d, char *name) {
	unsigned int i;
	for(i = 0; i < d->device.num_video_inputs; i++) {
		if (streq(name, d->inputs[i].name)) {
			return i + 1;
		}
	}
	return 0;
}

// Return 0 on error, number otherwise if it can parse the whole input
static unsigned int my_atoi(char *txt) {
	char *endptr = NULL;
	if (!txt)
		return 0;
	unsigned int ret = strtoul(txt, &endptr, 10);
	if (endptr == txt || *endptr)
		return 0;
	return ret;
}

void prepare_cmd_entry(struct videohub_data *d, struct vcmd_entry *e) {
	e->port_no1 = my_atoi(e->param1);
	e->port_no2 = my_atoi(e->param2);
	switch (e->cmd) {
	case CMD_INPUT_LABELS:
		if (e->port_no1 == 0 || e->port_no1 > d->device.num_video_inputs) {
			e->port_no1 = search_video_input_name(d, e->param1);
			if (!e->port_no1)
				die("Unknown input port number/name: %s", e->param1);
		}
		break;
	case CMD_OUTPUT_LABELS:
	case CMD_VIDEO_OUTPUT_LOCKS:
		if (e->port_no1 == 0 || e->port_no1 > d->device.num_video_outputs) {
			e->port_no1 = search_video_output_name(d, e->param1);
			if (!e->port_no1)
				die("Unknown output port number/name: %s", e->param1);
		}
		e->lock = d->outputs[e->port_no1 - 1].lock;
		break;
	case CMD_VIDEO_OUTPUT_ROUTING:
		if (e->port_no1 == 0 || e->port_no1 > d->device.num_video_outputs) {
			e->port_no1 = search_video_output_name(d, e->param1);
			if (!e->port_no1)
				die("Unknown output port number/name: %s", e->param1);
		}
		if (e->port_no2 == 0 || e->port_no2 > d->device.num_video_inputs) {
			e->port_no2 = search_video_input_name(d, e->param2);
			if (!e->port_no2)
				die("Unknown input port number/name: %s", e->param2);
		}
		break;
	case CMD_VIDEO_INPUT_STATUS:
	case CMD_VIDEO_OUTPUT_STATUS:
	case CMD_PROTOCOL_PREAMBLE:
	case CMD_VIDEOHUB_DEVICE:
	case CMD_PING:
	case CMD_ACK:
	case CMD_NAK:
		break;
	}
}

void format_cmd_text(struct vcmd_entry *e, char *buf, unsigned int bufsz) {
	switch (e->cmd) {
	case CMD_INPUT_LABELS:
		snprintf(buf, bufsz, "%s:\n%u %s\n\n", get_cmd_text(e->cmd),
			e->port_no1 - 1, e->param2);
		break;
	case CMD_OUTPUT_LABELS:
		snprintf(buf, bufsz, "%s:\n%u %s\n\n", get_cmd_text(e->cmd),
			e->port_no1 - 1, e->param2);
		break;
	case CMD_VIDEO_OUTPUT_LOCKS:
		snprintf(buf, bufsz, "%s:\n%u %s\n\n", get_cmd_text(e->cmd),
			e->port_no1 - 1, e->do_lock ? "O" : (e->lock == PORT_LOCKED_OTHER ? "F" : "U"));
		break;
	case CMD_VIDEO_OUTPUT_ROUTING:
		snprintf(buf, bufsz, "%s:\n%u %u\n\n", get_cmd_text(e->cmd),
			e->port_no1 - 1, e->port_no2 - 1);
		break;
	case CMD_VIDEO_INPUT_STATUS:
	case CMD_VIDEO_OUTPUT_STATUS:
	case CMD_PROTOCOL_PREAMBLE:
	case CMD_VIDEOHUB_DEVICE:
	case CMD_PING:
	case CMD_ACK:
	case CMD_NAK:
		break;
	}
}

void show_cmd(struct videohub_data *d, struct vcmd_entry *e) {
	const char *prefix = "videohub: ";
	switch (e->cmd) {
	case CMD_INPUT_LABELS:
		printf("%srename video input %d - \"%s\" to \"%s\"\n",
			prefix,
			e->port_no1, d->inputs[e->port_no1 - 1].name,
			e->param2
		);
		break;
	case CMD_OUTPUT_LABELS:
		printf("%srename video output %d - \"%s\" to \"%s\"\n",
			prefix,
			e->port_no1, d->outputs[e->port_no1 - 1].name,
			e->param2
		);
		break;
	case CMD_VIDEO_OUTPUT_LOCKS:
		printf("%s%s video output %d - \"%s\"\n",
			prefix,
			e->do_lock ? "lock" : (e->lock == PORT_LOCKED_OTHER ? "force unlock" : "unlock"),
			e->port_no1, d->outputs[e->port_no1 - 1].name
		);
		break;
	case CMD_VIDEO_OUTPUT_ROUTING:
		printf("%sset video output %d \"%s\" to read from input %d \"%s\"\n",
			prefix,
			e->port_no1, d->outputs[e->port_no1 - 1].name,
			e->port_no2, d->inputs [e->port_no2 - 1].name
		);
		break;
	case CMD_VIDEO_INPUT_STATUS:
	case CMD_VIDEO_OUTPUT_STATUS:
	case CMD_PROTOCOL_PREAMBLE:
	case CMD_VIDEOHUB_DEVICE:
	case CMD_PING:
	case CMD_ACK:
	case CMD_NAK:
		break;
	}
}
