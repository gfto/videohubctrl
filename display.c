/*
 * === Display functions ===
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
#include <string.h>

#include "data.h"
#include "cmd.h"
#include "util.h"
#include "display.h"

static void printf_line(int len) {
	int i;
	printf("  ");
	for (i = 0; i < len; i++)
		printf("-");
	printf("\n");
}

static char format_status(enum port_status status) {
	switch (status) {
	case S_UNKNOWN: return ' ';
	case S_BNC    : return 'B';
	case S_OPTICAL: return 'o';
	case S_THUNDERBOLT: return 'T';
	case S_NONE   : return 'x';
	case S_RS422  : return '4'; // For serial ports
	}
	return '?';
}

void print_device_info(struct videohub_data *d) {
	int len = 67;
	printf("Device info\n");
	printf_line(len);
	printf("  | %-26s | %-34s |\n", "Device address", d->dev_host);
	printf("  | %-26s | %-34s |\n", "Device port", d->dev_port);
	printf("  | %-26s | %-34s |\n", "Model name", d->device.model_name);
	if (d->device.friendly_name[0])
		printf("  | %-26s | %-34s |\n", "Friendly name", d->device.friendly_name);
	printf("  | %-26s | %-34s |\n", "Unique ID", d->device.unique_id);
	printf("  | %-26s | %-34s |\n", "Protocol", d->device.protocol_ver);
	printf("  | %-26s | %-34u |\n", "Video inputs", d->inputs.num);
	printf("  | %-26s | %-34u |\n", "Video outputs", d->outputs.num);
	if (d->serial.num)
		printf("  | %-26s | %-34u |\n", "Serial ports", d->serial.num);
	if (d->proc_units.num)
		printf("  | %-26s | %-34u |\n", "Video processing units", d->proc_units.num);
	if (d->mon_outputs.num)
		printf("  | %-26s | %-34u |\n", "Video monitoring outputs", d->mon_outputs.num);
	printf_line(len);
	printf("\n");
}

void print_device_video_inputs(struct videohub_data *d) {
	unsigned int i, r, len = 70;
	if (!d->inputs.num)
		return;
	printf("Video inputs\n");
	printf_line(len);
	printf("  | ### | %-24s | nn | %-24s | s |\n", "Video input name", "Routed to output");
	printf_line(len);
	for(i = 0; i < d->inputs.num; i++) {
		unsigned int num_outputs = 0, routed_to = 0;
		for(r = 0; r < d->outputs.num; r++) {
			if (d->outputs.port[r].routed_to == i) {
				num_outputs++;
				if (num_outputs == 1)
					routed_to = r; // The first output
			}
		}
		printf("  | %3d | %-24s | %2d | ", i + 1, d->inputs.port[i].name,
			 num_outputs);
		if (num_outputs == 0) {
			printf("%-24s | %c |\n", "-", format_status(d->inputs.port[i].status));
		} else {
			printf("%-24s | %c |\n",
				routed_to == NO_PORT ? "" : d->outputs.port[routed_to].name,
				format_status(d->inputs.port[i].status)
			);
			bool first_skipped = false;
			for(r = 0; r < d->outputs.num; r++) {
				if (d->outputs.port[r].routed_to == i) {
					if (!first_skipped) {
						first_skipped = true;
						continue;
					}
					printf("  | %3s | %-24s | %2s | %-24s | %c |\n",
						" ", " ", " ", d->outputs.port[r].name, ' ');
				}
			}
		}
	}
	printf_line(len);
	printf("\n");
}

static char port_lock_symbol(enum port_lock p) {
	switch(p) {
	case PORT_UNLOCKED    : return ' ';
	case PORT_LOCKED      : return 'O';
	case PORT_LOCKED_OTHER: return 'L';
	}
	return '?';
}

void print_device_video_outputs(struct videohub_data *d) {
	unsigned int i, len = 69;
	if (!d->outputs.num)
		return;
	printf("Video outputs\n");
	printf_line(len);
	printf("  | ### | x | %-24s | %-24s | s |\n", "Video output name", "Connected video input");
	printf_line(len);
	for(i = 0; i < d->outputs.num; i++) {
		printf("  | %3d | %c | %-24s | %-24s | %c |\n",
			i + 1,
			port_lock_symbol(d->outputs.port[i].lock),
			d->outputs.port[i].name,
			d->outputs.port[i].routed_to == NO_PORT ? "" : d->inputs.port[d->outputs.port[i].routed_to].name,
			format_status(d->outputs.port[i].status)
		);
	}
	printf_line(len);
	printf("\n");
}

void print_device_monitoring_outputs(struct videohub_data *d) {
	unsigned int i, len = 65;
	if (!d->mon_outputs.num)
		return;
	printf("Monitoring outputs\n");
	printf_line(len);
	printf("  | ### | x | %-24s | %-24s |\n", "Monitoring output name", "Connected video input");
	printf_line(len);
	for(i = 0; i < d->mon_outputs.num; i++) {
		printf("  | %3d | %c | %-24s | %-24s |\n",
			i + 1,
			port_lock_symbol(d->mon_outputs.port[i].lock),
			d->mon_outputs.port[i].name,
			d->mon_outputs.port[i].routed_to == NO_PORT ? "" : d->inputs.port[d->mon_outputs.port[i].routed_to].name
		);
	}
	printf_line(len);
	printf("\n");
}

static char *dir2opt(enum serial_dir dir) {
	switch (dir) {
	case DIR_CONTROL: return "in";
	case DIR_SLAVE  : return "out";
	case DIR_AUTO   : return "auto";
	}
	return "auto";
}

void print_device_serial_ports(struct videohub_data *d) {
	unsigned int i, len = 64;
	if (!d->serial.num)
		return;
	printf("Serial ports\n");
	printf_line(len);
	printf("  | ### | x | Dir  | %-18s | %-18s | s |\n", "Serial port", "Connected serial");
	printf_line(len);
	for(i = 0; i < d->serial.num; i++) {
		printf("  | %3d | %c | %4s | %-18s | %-18s | %c |\n",
			i + 1,
			port_lock_symbol(d->serial.port[i].lock),
			dir2opt(d->serial.port[i].direction),
			d->serial.port[i].name,
			d->serial.port[i].routed_to == NO_PORT ? "" : d->serial.port[d->serial.port[i].routed_to].name,
			format_status(d->serial.port[i].status)
		);
	}
	printf_line(len);
	printf("\n");
}

void print_device_processing_units(struct videohub_data *d) {
	unsigned int i, len = 44;
	if (!d->proc_units.num)
		return;
	printf("Processing units\n");
	printf_line(len);
	printf("  | Proc Unit | x | %-24s |\n", "Connected video input");
	printf_line(len);
	for(i = 0; i < d->proc_units.num; i++) {
		printf("  | %9d | %c | %-24s |\n",
			i + 1,
			port_lock_symbol(d->proc_units.port[i].lock),
			d->proc_units.port[i].routed_to == NO_PORT ? "" : d->inputs.port[d->proc_units.port[i].routed_to].name
		);
	}
	printf_line(len);
	printf("\n");
}

static void __print_opt(struct videohub_data *d, enum vcmd vcmd) {
	unsigned int i, last = 0;
	struct videohub_commands *v = &videohub_commands[vcmd];
	struct port_set *s_port = !v->ports1 ? NULL : (void *)d + v->ports1;
	const char *p = v->opt_prefix;
	for(i = 0; i < s_port->num; i++) {
		switch (v->type) {
		case PARSE_LABEL:
			printf("  --%s-name %3d \"%s\" \\\n", p, i + 1, s_port->port[i].name);
			break;
		case PARSE_ROUTE:
			if (s_port->port[i].routed_to == NO_PORT)
				continue;
			printf("  --%s-input %3d %3d \\\n", p, i + 1, s_port->port[i].routed_to + 1);
			break;
		case PARSE_LOCK:
			last = i + 1 < s_port->num;
			if (s_port->port[i].lock != PORT_UNLOCKED) {
				printf("  --%s-unlock %3d --%s-lock %3d%s\n",
					p, i + 1, p, i + 1, last ? " \\" : "");
			} else {
				printf("  --%s-unlock %3d%s\n", p, i + 1, last ? " \\" : "");
			}
			break;
		case PARSE_DIR:
			printf("  --%s-dir %3d %s \\\n", p, i + 1, dir2opt(s_port->port[i].direction));
			break;
		default: break;
		}
	}
}

void print_device_backup(struct videohub_data *d) {
	unsigned int i;
	printf("videohubctrl \\\n");
	for (i = 0; i < NUM_COMMANDS; i++) {
		if (videohub_commands[i].type == PARSE_LABEL)
			__print_opt(d, videohub_commands[i].cmd);
	}
	for (i = 0; i < NUM_COMMANDS; i++) {
		if (videohub_commands[i].type == PARSE_ROUTE)
			__print_opt(d, videohub_commands[i].cmd);
	}
	for (i = 0; i < NUM_COMMANDS; i++) {
		if (videohub_commands[i].type == PARSE_DIR)
			__print_opt(d, videohub_commands[i].cmd);
	}
	for (i = 0; i < NUM_COMMANDS; i++) {
		if (videohub_commands[i].type == PARSE_LOCK)
			__print_opt(d, videohub_commands[i].cmd);
	}
	printf("\n");
}
