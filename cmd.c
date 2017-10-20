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
#include <stddef.h>
#include <string.h>

#include "data.h"
#include "cmd.h"
#include "util.h"

const char *videohub_commands_text[NUM_COMMANDS] = {
	[CMD_PROTOCOL_PREAMBLE]    = "PROTOCOL PREAMBLE",
	[CMD_VIDEOHUB_DEVICE]      = "VIDEOHUB DEVICE",
	[CMD_CONFIGURATION]        = "CONFIGURATION",
	[CMD_INPUT_LABELS]         = "INPUT LABELS",
	[CMD_OUTPUT_LABELS]        = "OUTPUT LABELS",
	[CMD_VIDEO_OUTPUT_LOCKS]   = "VIDEO OUTPUT LOCKS",
	[CMD_VIDEO_OUTPUT_ROUTING] = "VIDEO OUTPUT ROUTING",
	[CMD_VIDEO_INPUT_STATUS]   = "VIDEO INPUT STATUS",
	[CMD_VIDEO_OUTPUT_STATUS]  = "VIDEO OUTPUT STATUS",
	[CMD_MONITORING_OUTPUT_LABELS]  = "MONITORING OUTPUT LABELS",
	[CMD_MONITORING_OUTPUT_LOCKS]   = "MONITORING OUTPUT LOCKS",
	[CMD_MONITORING_OUTPUT_ROUTING] = "VIDEO MONITORING OUTPUT ROUTING",
	[CMD_SERIAL_PORT_LABELS]     = "SERIAL PORT LABELS",
	[CMD_SERIAL_PORT_ROUTING]    = "SERIAL PORT ROUTING",
	[CMD_SERIAL_PORT_LOCKS]      = "SERIAL PORT LOCKS",
	[CMD_SERIAL_PORT_STATUS]     = "SERIAL PORT STATUS",
	[CMD_SERIAL_PORT_DIRECTIONS] = "SERIAL PORT DIRECTIONS",
	[CMD_PROCESSING_UNIT_ROUTING]= "PROCESSING UNIT ROUTING",
	[CMD_PROCESSING_UNIT_LOCKS]  = "PROCESSING UNIT LOCKS",
	[CMD_FRAME_LABELS]           = "FRAME LABELS",
	[CMD_FRAME_BUFFER_ROUTING]   = "FRAME BUFFER ROUTING",
	[CMD_FRAME_BUFFER_LOCKS]     = "FRAME BUFFER LOCKS",
	[CMD_PING]                 = "PING",
	[CMD_ACK]                  = "ACK",
	[CMD_NAK]                  = "NAK",
};

#define OFS(X) offsetof(struct videohub_data, X)

struct videohub_commands videohub_commands[NUM_COMMANDS] = {
	[CMD_PROTOCOL_PREAMBLE]    = { .cmd = CMD_PROTOCOL_PREAMBLE   , .type = PARSE_CUSTOM },
	[CMD_VIDEOHUB_DEVICE]      = { .cmd = CMD_VIDEOHUB_DEVICE     , .type = PARSE_CUSTOM },
	[CMD_CONFIGURATION]        = { .cmd = CMD_CONFIGURATION       , .type = PARSE_CUSTOM },
	[CMD_INPUT_LABELS]         = { .cmd = CMD_INPUT_LABELS        , .type = PARSE_LABEL,
		.ports1 = OFS(inputs),
		.port_id1 = "video input",
		.opt_prefix = "in",
	},
	[CMD_OUTPUT_LABELS]        = { .cmd = CMD_OUTPUT_LABELS       , .type = PARSE_LABEL,
		.ports1 = OFS(outputs),
		.port_id1 = "video output",
		.opt_prefix = "out",
	},
	[CMD_VIDEO_OUTPUT_LOCKS]   = { .cmd = CMD_VIDEO_OUTPUT_LOCKS  , .type = PARSE_LOCK,
		.ports1 = OFS(outputs),
		.port_id1 = "video output",
		.opt_prefix = "out",
	},
	[CMD_VIDEO_OUTPUT_ROUTING] = { .cmd = CMD_VIDEO_OUTPUT_ROUTING, .type = PARSE_ROUTE,
		.ports1 = OFS(outputs),
		.ports2 = OFS(inputs),
		.port_id1 = "video output",
		.port_id2 = "video input",
		.opt_prefix = "out",
	},
	[CMD_VIDEO_INPUT_STATUS]   = { .cmd = CMD_VIDEO_INPUT_STATUS  , .type = PARSE_STATUS,
		.ports1 = OFS(inputs),
		.port_id1 = "video input",
		.opt_prefix = "in",
	},
	[CMD_VIDEO_OUTPUT_STATUS]  = { .cmd = CMD_VIDEO_OUTPUT_STATUS , .type = PARSE_STATUS,
		.ports1 = OFS(outputs),
		.port_id1 = "video output",
		.opt_prefix = "out",
	},
	[CMD_MONITORING_OUTPUT_LABELS]  = { .cmd = CMD_MONITORING_OUTPUT_LABELS , .type = PARSE_LABEL,
		.ports1 = OFS(mon_outputs),
		.port_id1 = "monitoring output",
		.opt_prefix = "mon",
	},
	[CMD_MONITORING_OUTPUT_LOCKS]   = { .cmd = CMD_MONITORING_OUTPUT_LOCKS  , .type = PARSE_LOCK,
		.ports1 = OFS(mon_outputs),
		.port_id1 = "monitoring output",
		.opt_prefix = "mon",
	},
	[CMD_MONITORING_OUTPUT_ROUTING] = { .cmd = CMD_MONITORING_OUTPUT_ROUTING, .type = PARSE_ROUTE,
		.ports1 = OFS(mon_outputs),
		.ports2 = OFS(inputs),
		.port_id1 = "monitoring output",
		.port_id2 = "video input",
		.opt_prefix = "mon",
	},
	[CMD_SERIAL_PORT_LABELS]  = { .cmd = CMD_SERIAL_PORT_LABELS, .type = PARSE_LABEL,
		.ports1 = OFS(serial),
		.port_id1 = "serial",
		.opt_prefix = "ser",
	},
	[CMD_SERIAL_PORT_LOCKS]   = { .cmd = CMD_SERIAL_PORT_LOCKS  , .type = PARSE_LOCK,
		.ports1 = OFS(serial),
		.port_id1 = "serial",
		.opt_prefix = "ser",
	},
	[CMD_SERIAL_PORT_ROUTING] = { .cmd = CMD_SERIAL_PORT_ROUTING, .type = PARSE_ROUTE,
		.ports1 = OFS(serial),
		.ports2 = OFS(serial),
		.port_id1 = "serial",
		.port_id2 = "serial",
		.opt_prefix = "ser",
		.allow_disconnect = true,
	},
	[CMD_SERIAL_PORT_STATUS]  = { .cmd = CMD_SERIAL_PORT_STATUS , .type = PARSE_STATUS,
		.ports1 = OFS(serial),
		.port_id1 = "serial",
		.opt_prefix = "ser",
	},
	[CMD_SERIAL_PORT_DIRECTIONS] = { .cmd = CMD_SERIAL_PORT_DIRECTIONS, .type = PARSE_DIR,
		.ports1 = OFS(serial),
		.port_id1 = "serial",
		.opt_prefix = "ser",
	},
	[CMD_PROCESSING_UNIT_ROUTING] = { .cmd = CMD_PROCESSING_UNIT_ROUTING, .type = PARSE_ROUTE,
		.ports1 = OFS(proc_units),
		.ports2 = OFS(inputs),
		.port_id1 = "proc unit",
		.port_id2 = "input",
		.opt_prefix = "pu",
		.allow_disconnect = true,
	},
	[CMD_PROCESSING_UNIT_LOCKS] = { .cmd = CMD_PROCESSING_UNIT_LOCKS, .type = PARSE_LOCK,
		.ports1 = OFS(proc_units),
		.port_id1 = "proc unit",
		.opt_prefix = "pu",
	},
	[CMD_FRAME_LABELS]  = { .cmd = CMD_FRAME_LABELS, .type = PARSE_LABEL,
		.ports1 = OFS(frames),
		.port_id1 = "frame",
		.opt_prefix = "fr",
	},
	[CMD_FRAME_BUFFER_ROUTING] = { .cmd = CMD_FRAME_BUFFER_ROUTING, .type = PARSE_ROUTE,
		.ports1 = OFS(frames),
		.ports2 = OFS(outputs),
		.port_id1 = "frame",
		.port_id2 = "output",
		.opt_prefix = "fr",
		.allow_disconnect = true,
	},
	[CMD_FRAME_BUFFER_LOCKS] = { .cmd = CMD_FRAME_BUFFER_LOCKS, .type = PARSE_LOCK,
		.ports1 = OFS(frames),
		.port_id1 = "frame",
		.opt_prefix = "fr",
	},
	[CMD_PING]                 = { .cmd = CMD_PING                , .type = PARSE_NONE },
	[CMD_ACK]                  = { .cmd = CMD_ACK                 , .type = PARSE_NONE },
	[CMD_NAK]                  = { .cmd = CMD_NAK                 , .type = PARSE_NONE },
};

static char *parse_text(char *line, char *cmd) {
	char *parsed_text = strstr(line, cmd);
	if (parsed_text == line) {
		return parsed_text + strlen(cmd);
	}
	return NULL;
}

bool parse_command(struct videohub_data *d, char *cmd) {
	unsigned int i;
	bool ret = true;
	if (!strlen(cmd))
		return false;
	struct videohub_commands *v = NULL;
	const char *cmd_txt = NULL;
	for (i = 0; i < NUM_COMMANDS; i++) {
		const char *cmd_text = videohub_commands_text[i];
		if (strstr(cmd, cmd_text) == cmd) {
			v = &videohub_commands[i];
			cmd_txt = videohub_commands_text[i];
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

	d("debug: Got '%s' command.\n", cmd_txt);
	if (debug > 1)
		d("----\n%s\n----\n", cmd);

	struct port_set *s_port = !v->ports1 ? NULL : (void *)d + v->ports1;
	struct port_set *d_port = !v->ports2 ? NULL : (void *)d + v->ports2;

	char *p, *cmd_data = xstrdup( cmd + strlen(cmd_txt) + 2 ); // +2 to compensate for :\n at the end of the command
	// Split line by line
	char *line, *saveptr = NULL;
	for(i = 0, line = strtok_r(cmd_data, "\n", &saveptr); line; line = strtok_r(NULL, "\n", &saveptr), i++) {

		// Parse command data response
		unsigned int port_num = 0;
		unsigned int dest_port_num = 0;
		char *port_data = NULL;

		if (v->type != PARSE_NONE && v->type != PARSE_CUSTOM) {
			port_data = strchr(line, ' ');
			if (!port_data)
				continue;
			port_data[0] = '\0'; // Separate port_num from port_data
			port_data++;
			port_num = strtoul(line, NULL, 10);
			if (port_num + 1 > s_port->num) {
				q("WARNING: %s: invalid %s port %u (valid 0..%u)\n", cmd_txt,
				  v->port_id1, port_num, s_port->num - 1);
				continue;
			}
		}

		switch (v->type) {
		case PARSE_LABEL:
			snprintf(s_port->port[port_num].name, sizeof(s_port->port[port_num].name), "%s", port_data);
			break;
		case PARSE_STATUS:
			s_port->port[port_num].status = S_UNKNOWN;
			bool invalid_status = false;
			if (v->cmd == CMD_SERIAL_PORT_STATUS) {
				if (streq("RS422", port_data))        s_port->port[port_num].status = S_RS422;
				else if (streq("None", port_data))    s_port->port[port_num].status = S_NONE;
				else invalid_status = true;
			} else {
				if (streq("BNC", port_data))          s_port->port[port_num].status = S_BNC;
				else if (streq("Optical", port_data)) s_port->port[port_num].status = S_OPTICAL;
				else if (streq("None", port_data))    s_port->port[port_num].status = S_NONE;
				else if (streq("Thunderbolt", port_data)) s_port->port[port_num].status = S_THUNDERBOLT;
				else invalid_status = true;
			}
			if (invalid_status) {
				q("WARNING: %s command returned unknown status: '%s'\n", cmd_txt, port_data);
				q("Please report this line to author's email: georgi@unixsol.org\n");
			}
			break;
		case PARSE_DIR:
			s_port->port[port_num].direction = DIR_AUTO;
			if (streq("control", port_data))      s_port->port[port_num].direction = DIR_CONTROL;
			else if (streq("slave", port_data))   s_port->port[port_num].direction = DIR_SLAVE;
			else if (streq("auto", port_data))    s_port->port[port_num].direction = DIR_AUTO;
			else {
				q("WARNING: %s command returned unknown direction: '%s'\n", cmd_txt, port_data);
				q("Please report this line to author's email: georgi@unixsol.org\n");
			}
			break;
		case PARSE_ROUTE:
			dest_port_num = strtoul(port_data, NULL, 10);
			if (dest_port_num == NO_PORT) {
				if (v->allow_disconnect) {
					s_port->port[port_num].routed_to = dest_port_num;
					continue;
				} else {
					dest_port_num = port_num;
				}
			}
			if (dest_port_num + 1 > d_port->num) {
				q("WARNING: %s: invalid %s port %u (valid 0..%u)\n", cmd_txt,
				  v->port_id2, dest_port_num, d_port->num - 1);
				continue;
			}
			s_port->port[port_num].routed_to = dest_port_num;
			break;
		case PARSE_LOCK:
			switch (port_data[0]) {
			case 'O': s_port->port[port_num].lock = PORT_LOCKED; break;
			case 'L': s_port->port[port_num].lock = PORT_LOCKED_OTHER; break;
			default : s_port->port[port_num].lock = PORT_UNLOCKED; break;
			}
			break;
		default: break;
		}

		// Parse custom commands
		switch (v->cmd) {
		case CMD_PROTOCOL_PREAMBLE:
			if ((p = parse_text(line, "Version: ")))
				snprintf(d->device.protocol_ver, sizeof(d->device.protocol_ver), "%s", p);
			break;
		case CMD_VIDEOHUB_DEVICE:
			if ((p = parse_text(line, "Device present: "))) {
				d->device.dev_present = streq(p, "true");
				d->device.needs_fw_update = streq(p, "needs_update");
			}
			else if ((p = parse_text(line, "Model name: ")))
				snprintf(d->device.model_name, sizeof(d->device.model_name), "%s", p);
			else if ((p = parse_text(line, "Friendly name: ")))
				snprintf(d->device.friendly_name, sizeof(d->device.friendly_name), "%s", p);
			else if ((p = parse_text(line, "Unique ID: ")))
				snprintf(d->device.unique_id, sizeof(d->device.unique_id) , "%s", p);
			else if ((p = parse_text(line, "Video inputs: ")))
				d->inputs.num = strtoul(p, NULL, 10);
			else if ((p = parse_text(line, "Video processing units: ")))
				d->proc_units.num = strtoul(p, NULL, 10);
			else if ((p = parse_text(line, "Video outputs: ")))
				d->outputs.num = strtoul(p, NULL, 10);
			else if ((p = parse_text(line, "Video monitoring outputs: ")))
				d->mon_outputs.num = strtoul(p, NULL, 10);
			else if ((p = parse_text(line, "Serial ports: "))) {
				d->serial.num = strtoul(p, NULL, 10);
				d->frames.num = d->serial.num;
			} else {
				q("WARNING: VIDEOHUB DEVICE command sent unknown line: '%s'\n", line);
				q("Please report this line to author's email: georgi@unixsol.org\n");
			}
			break;
		case CMD_CONFIGURATION:
			if ((p = parse_text(line, "Take Mode: "))) {
				d->device.conf_take_mode = streq(p, "true");
			}
		case CMD_NAK:
			ret = false;
			break;
		default: break;
		}
	}
	free(cmd_data);
	return ret;
}

int parse_text_buffer(struct videohub_data *d, char *cmd_buffer) {
	// The buffer contains only one command, no splitting is needed
	if (!strstr(cmd_buffer, "\n\n"))
		return parse_command(d, cmd_buffer);
	// Split commands and parse them one by one
	int ok_commands = 0;
	char *buf_copy = xstrdup(cmd_buffer);
	char *newcmd, *cmd = buf_copy;
	while(1) {
		newcmd = strstr(cmd, "\n\n"); // Find next command
		if (!newcmd) {
			if (parse_command(d, cmd)) // Parse current command
				ok_commands++;
			break;
		}
		newcmd[0] = '\0'; // Terminate previous command
		if (parse_command(d, cmd)) // Parse previous command
			ok_commands++;
		cmd = newcmd + 2; // Advance cmd to the next command
	}
	free(buf_copy);
	return ok_commands;
}

// Try to find port with certain name, return 0 on not found, pos + 1 is found
static int get_port_by_name(struct port_set *p, char *name) {
	unsigned int i;
	for(i = 0; i < p->num; i++) {
		if (streq(name, p->port[i].name)) {
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

static void init_port_number(struct vcmd_param *p, struct port_set *port, const char *port_id) {
	p->port_no = my_atoi(p->param);
	if (!port) {
		die("impossible! port == NULL");
		return;
	}
	if (p->port_no == 0 || p->port_no > port->num) {
		p->port_no = get_port_by_name(port, p->param);
		if (!p->port_no)
			die("Unknown %s port number/name: %s", port_id, p->param);
	}
}

void prepare_cmd_entry(struct videohub_data *d, struct vcmd_entry *e) {
	struct port_set *s_port = !e->cmd->ports1 ? NULL : (void *)d + e->cmd->ports1;
	struct port_set *d_port = !e->cmd->ports2 ? NULL : (void *)d + e->cmd->ports2;

	if (e->cmd->type == PARSE_CUSTOM)
		return;

	// All command types needs parsing of the "source port"
	init_port_number(&e->p1, s_port, e->cmd->port_id1);

	// ROUTE type needs parsing of the "destination port"
	if (e->cmd->type == PARSE_ROUTE) {
		init_port_number(&e->p2, d_port, e->cmd->port_id2);
	}

	// Allow port_noX to be used as index into ->port[]
	e->p1.port_no -= 1;
	e->p2.port_no -= 1;
	if (e->clear_port)
		e->p2.port_no = NO_PORT;

	if (e->cmd->type == PARSE_LOCK) {
		e->lock = s_port->port[e->p1.port_no].lock;
	}
}

static char *dir2cmd(enum serial_dir dir) {
	switch (dir) {
	case DIR_CONTROL: return "control";
	case DIR_SLAVE  : return "slave";
	case DIR_AUTO   : return "auto";
	}
	return "auto";
}

static char *dir2txt(enum serial_dir dir) {
	switch (dir) {
	case DIR_CONTROL: return "IN (Workstation)";
	case DIR_SLAVE  : return "OUT (Deck)";
	case DIR_AUTO   : return "AUTO";
	}
	return "AUTO";
}

void format_cmd_text(struct vcmd_entry *e, char *buf, unsigned int bufsz) {
	if (e->cmd->cmd == CMD_VIDEOHUB_DEVICE) {
		snprintf(buf, bufsz, "%s:\n%s: %s\n\n", videohub_commands_text[e->cmd->cmd],
			e->p1.param, e->p2.param);
		return;
	}
	switch (e->cmd->type) {
	case PARSE_LABEL:
		snprintf(buf, bufsz, "%s:\n%u %s\n\n", videohub_commands_text[e->cmd->cmd],
			e->p1.port_no, e->p2.param);
		break;
	case PARSE_LOCK:
		snprintf(buf, bufsz, "%s:\n%u %s\n\n", videohub_commands_text[e->cmd->cmd],
			e->p1.port_no, e->do_lock ? "O" : (e->lock == PORT_LOCKED_OTHER ? "F" : "U"));
		break;
	case PARSE_ROUTE:
		snprintf(buf, bufsz, "%s:\n%u %d\n\n", videohub_commands_text[e->cmd->cmd],
			e->p1.port_no, (e->p2.port_no == NO_PORT ? -1 : (int)e->p2.port_no));
		break;
	case PARSE_DIR:
		snprintf(buf, bufsz, "%s:\n%u %s\n\n", videohub_commands_text[e->cmd->cmd],
			e->p1.port_no, dir2cmd(e->direction));
		break;
	default: break;
	}
}

void show_cmd(struct videohub_data *d, struct vcmd_entry *e) {
	struct port_set *s_port = !e->cmd->ports1 ? NULL : (void *)d + e->cmd->ports1;
	struct port_set *d_port = !e->cmd->ports2 ? NULL : (void *)d + e->cmd->ports2;
	const char *prefix = "videohub: ";
	if (e->cmd->cmd == CMD_VIDEOHUB_DEVICE) {
		printf("%sset device \"%s\" to \"%s\"\n",
			prefix,
			e->p1.param,
			e->p2.param
		);
		return;
	}
	switch (e->cmd->type) {
	case PARSE_LABEL:
		printf("%srename %s %d \"%s\" to \"%s\"\n",
			prefix,
			e->cmd->port_id1,
			e->p1.port_no + 1, s_port->port[e->p1.port_no].name,
			e->p2.param
		);
		break;
	case PARSE_LOCK:
		printf("%s%s %s %d \"%s\"\n",
			prefix,
			e->do_lock ? "lock" : (e->lock == PORT_LOCKED_OTHER ? "force unlock" : "unlock"),
			e->cmd->port_id1,
			e->p1.port_no + 1, s_port->port[e->p1.port_no].name
		);
		break;
	case PARSE_ROUTE:
		if (e->p2.port_no == NO_PORT) {
			printf("%sdisconnect %s %d \"%s\"\n",
				prefix,
				e->cmd->port_id1,
				e->p1.port_no + 1, s_port->port[e->p1.port_no].name
			);
			break;
		}
		if (e->cmd->allow_disconnect) {
			printf("%sconnect %s %d \"%s\" to %s %d \"%s\"\n",
				prefix,
				e->cmd->port_id1,
				e->p1.port_no + 1, s_port->port[e->p1.port_no].name,
				e->cmd->port_id2,
				e->p2.port_no + 1, d_port->port [e->p2.port_no].name
			);
			break;
		}
		if (e->reversed_args) {
			printf("%sset %s %d \"%s\" to go out of %s %d \"%s\"\n",
				prefix,
				e->cmd->port_id2,
				e->p2.port_no + 1, d_port->port[e->p2.port_no].name,
				e->cmd->port_id1,
				e->p1.port_no + 1, s_port->port [e->p1.port_no].name
			);
			break;
		}
		printf("%sset %s %d \"%s\" to read from %s %d \"%s\"\n",
			prefix,
			e->cmd->port_id1,
			e->p1.port_no + 1, s_port->port[e->p1.port_no].name,
			e->cmd->port_id2,
			e->p2.port_no + 1, d_port->port [e->p2.port_no].name
		);
		break;
	case PARSE_DIR:
		printf("%sset %s %d \"%s\" direction to %s\n",
			prefix,
			e->cmd->port_id1,
			e->p1.port_no + 1, s_port->port[e->p1.port_no].name,
			dir2txt(e->direction)
		);
		break;
	default: break;
	}
}
