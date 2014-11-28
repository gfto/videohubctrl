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

/*
Example response from videohub (on connect)
===========================================
PROTOCOL PREAMBLE:
Version: 2.4

VIDEOHUB DEVICE:
Device present: true
Model name: Blackmagic Micro Videohub
Unique ID: 7c2e0d021714
Video inputs: 16
Video processing units: 0
Video outputs: 16
Video monitoring outputs: 0
Serial ports: 0

INPUT LABELS:
0 Windows 1
1 Windows 2
2 Windows 3
3 Windows 4 HD
4 Input 5
5 Input 6
6 Input 7
7 Input 8
8 Input 9
9 Input 10
10 Input 11
11 DPlay1
12 DPlay2
13 Input 14
14 Input 15
15 Loopback

OUTPUT LABELS:
0 Enc1 1
1 Enc1 2
2 Enc1 3
3 Enc1 4
4 Output 5
5 Output 6
6 Output 7
7 Output 8
8 Enc2 1
9 Output 10
10 Output 11
11 Denc
12 Output 13
13 Output 14
14 Output 15
15 Loopback

VIDEO OUTPUT LOCKS:
0 L
1 L
2 L
3 L
4 U
5 U
6 U
7 U
8 L
9 U
10 U
11 L
12 L
13 O
14 U
15 O

VIDEO OUTPUT ROUTING:
0 2
1 1
2 0
3 0
4 4
5 5
6 6
7 7
8 3
9 3
10 10
11 12
12 11
13 13
14 14
15 15

*/

static struct videohub_commands {
	enum vcmd	cmd;
	const char	*txt;
} videohub_commands[] = {
	{ CMD_PROTOCOL_PREAMBLE,       "PROTOCOL PREAMBLE" },
	{ CMD_VIDEOHUB_DEVICE,         "VIDEOHUB DEVICE" },
	{ CMD_INPUT_LABELS,            "INPUT LABELS" },
	{ CMD_OUTPUT_LABELS,           "OUTPUT LABELS" },
	{ CMD_VIDEO_OUTPUT_LOCKS,      "VIDEO OUTPUT LOCKS" },
	{ CMD_VIDEO_OUTPUT_ROUTING,    "VIDEO OUTPUT ROUTING" },
	{ CMD_PING,                    "PING" },
	{ CMD_ACK,                     "ACK" },
	{ CMD_NAK,                     "NAK" },
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
		if (!quiet) {
			fprintf(stderr, "WARNING: Videohub sent unknown command!\n");
			fprintf(stderr, "         Please report this command to author's email: georgi@unixsol.org\n");
			fprintf(stderr, "         You may use -q or --quiet to suppress the message.\n");
			fprintf(stderr, "---------8<-----------8<----------- cut here ---------8<------------8<---------\n");
			fprintf(stderr, "%s\n", cmd);
			fprintf(stderr, "---------8<-----------8<----------- cut here ---------8<------------8<---------\n");
		}
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
		bool valid_slot = false;
		unsigned int slot_pos = 0;
		char *slot_data = NULL;
		switch (v->cmd) {
		case CMD_INPUT_LABELS:
		case CMD_OUTPUT_LABELS:
		case CMD_VIDEO_OUTPUT_LOCKS:
		case CMD_VIDEO_OUTPUT_ROUTING:
			slot_data = strchr(line, ' ');
			if (slot_data) {
				slot_data[0] = '\0'; // Separate slot_pos from slot_data
				slot_data++;
				slot_pos = strtoul(line, NULL, 10);
				if (slot_pos < ARRAY_SIZE(data->outputs))
					valid_slot = true;
			}
			break;
		default:
			break;
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
			if (valid_slot)
				snprintf(data->inputs[slot_pos].name, sizeof(data->inputs[slot_pos].name), "%s", slot_data);
			break;

		case CMD_OUTPUT_LABELS:
			if (valid_slot)
				snprintf(data->outputs[slot_pos].name, sizeof(data->inputs[slot_pos].name), "%s", slot_data);
			break;

		case CMD_VIDEO_OUTPUT_LOCKS:
			if (valid_slot) {
				// L is lock owned by somebody else (set from other IP address)
				// O is lock owned by us (set from our IP address)
				if (slot_data[0] == 'L' || slot_data[0] == 'O') {
					data->outputs[slot_pos].locked = true;
					if (slot_data[0] == 'L')
						data->outputs[slot_pos].locked_other = true;
					else
						data->outputs[slot_pos].locked_other = false;
				} else {
					data->outputs[slot_pos].locked = false;
				}
			}
			break;

		case CMD_VIDEO_OUTPUT_ROUTING:
			if (valid_slot) {
				unsigned int dest_pos = strtoul(slot_data, NULL, 10);
				if (dest_pos < ARRAY_SIZE(data->inputs))
					data->outputs[slot_pos].routed_to = dest_pos;
			}
			break;

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
	char *bcopy = xstrdup(cmd_buffer);
	char *newcmd, *cmd = bcopy;
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
	free(bcopy);
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
			e->locked_other = d->outputs[e->port_no1 - 1].locked_other;
		} else {
			e->locked_other = d->outputs[e->port_no1 - 1].locked_other;
		}
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
			e->port_no1 - 1, e->do_lock ? "O" : (e->locked_other ? "F" : "U"));
		break;
	case CMD_VIDEO_OUTPUT_ROUTING:
		snprintf(buf, bufsz, "%s:\n%u %u\n\n", get_cmd_text(e->cmd),
			e->port_no1 - 1, e->port_no2 - 1);
		break;
	case CMD_PROTOCOL_PREAMBLE:
	case CMD_VIDEOHUB_DEVICE:
	case CMD_PING:
	case CMD_ACK:
	case CMD_NAK:
		break;
	}
}
