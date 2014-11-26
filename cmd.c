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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"
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
13 U
14 U
15 U

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

enum vcmd {
	CMD_PROTOCOL_PREAMBLE,
	CMD_VIDEOHUB_DEVICE,
	CMD_INPUT_LABELS,
	CMD_OUTPUT_LABELS,
	CMD_VIDEO_OUTPUT_LOCKS,
	CMD_VIDEO_OUTPUT_ROUTING,
	CMD_PING,
};

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
};

static char *parse_text(char *line, char *cmd) {
	char *parsed_text = strstr(line, cmd);
	if (parsed_text == line) {
		return parsed_text + strlen(cmd);
	}
	return NULL;
}

bool parse_command(struct videohub_data *data, char *cmd) {
	unsigned int i;
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

	v("verbose: Got '%s' command.\n", v->txt);
	if (verbose > 1)
		v("----\n%s\n----\n", cmd);

	bool cmd_response = false;
	char *p, *cmd_data = xstrdup( cmd + strlen(v->txt) + 2 ); // +2 to compensate for :\n at the end of the command
	// Split line by line
	char *line, *saveptr = NULL;
	for(i = 0, line = strtok_r(cmd_data, "\n", &saveptr); line; line = strtok_r(NULL, "\n", &saveptr), i++) {
		if (i == 0) { // Handle command response
			if (streq(line, "NAK"))
				return false;
			if (streq(line, "ACK")) {
				cmd_response = true;
				continue;
			}
		}

		if (i == 1 && cmd_response) { // The command must match
			if (strstr(line, v->txt) != line)
				return false;
			continue;
		}

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
				if (slot_pos < ARRAY_SIZE(data->inputs))
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
			if ((p = parse_text(line, "Device present: ")))
				data->device.dev_present = streq(p, "true");

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
			if (valid_slot)
				data->outputs[slot_pos].locked = slot_data[0] == 'L' ? true : false;
			break;

		case CMD_VIDEO_OUTPUT_ROUTING:
			if (valid_slot) {
				unsigned int dest_pos = strtoul(slot_data, NULL, 10);
				if (dest_pos < ARRAY_SIZE(data->outputs))
					data->outputs[slot_pos].routed_to = dest_pos;
			}
			break;

		case CMD_PING:
			// Do nothing, we just get ACK without any data
			break;
		}
	}
	free(cmd_data);

	/* Check if everything is within limits */
	switch (v->cmd) {
	case CMD_PROTOCOL_PREAMBLE:
		if (!streq(data->device.protocol_ver, "2.4")) {
			if (!quiet) {
				fprintf(stderr, "WARNING: Device protocol is %s but this program is tested with 2.4 only.\n",
					data->device.protocol_ver);
				fprintf(stderr, "         Please report successes/failures to author's email: georgi@unixsol.org\n");
				fprintf(stderr, "         You may use -q or --quiet to suppress the message.\n");
			}
		}
		break;
	case CMD_VIDEOHUB_DEVICE:
		if (!data->device.dev_present) {
			die("Device reports that it's not present.");
		}
		if (data->device.num_video_inputs > ARRAY_SIZE(data->inputs)) {
			die("The device supports %d inputs. Recompile the program with more MAX_INPUTS (currently %d)",
				data->device.num_video_inputs, MAX_INPUTS);
		}
		if (data->device.num_video_outputs > ARRAY_SIZE(data->outputs)) {
			die("The device supports %d outputs. Recompile the program with more MAX_OUTPUTS (currently %d)\n",
				data->device.num_video_outputs, MAX_OUTPUTS);
		}
	default:
		break;
	}

	return true;
}
