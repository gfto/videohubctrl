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
#include "util.h"
#include "display.h"

static void printf_line(int len) {
	int i;
	printf("  ");
	for (i = 0; i < len; i++)
		printf("-");
	printf("\n");
}

static char format_status(char *status) {
	if (!strlen(status))          return ' ';
	if (streq(status, "BNC"))     return 'B';
	if (streq(status, "Optical")) return 'o';
	if (streq(status, "None"))    return 'x';
	return '?';
}

void print_device_info(struct videohub_data *d) {
	int len = 59;
	printf("Device info\n");
	printf_line(len);
	printf("  | %-26s | %-26s |\n", "Device address", d->dev_host);
	printf("  | %-26s | %-26s |\n", "Device port", d->dev_port);
	printf("  | %-26s | %-26s |\n", "Model name", d->device.model_name);
	printf("  | %-26s | %-26s |\n", "Unique ID", d->device.unique_id);
	printf("  | %-26s | %-26s |\n", "Protocol", d->device.protocol_ver);
	printf("  | %-26s | %-26u |\n", "Video inputs", d->device.num_video_inputs);
	printf("  | %-26s | %-26u |\n", "Video outputs", d->device.num_video_outputs);
	if (d->device.num_serial_ports)
		printf("  | %-26s | %-26u |\n", "Serial ports", d->device.num_serial_ports);
	if (d->device.num_video_processing_units)
		printf("  | %-26s | %-26u |\n", "Video processing units", d->device.num_video_processing_units);
	if (d->device.num_video_monitoring_outputs)
		printf("  | %-26s | %-26u |\n", "Video monitoring outputs", d->device.num_video_monitoring_outputs);
	printf_line(len);
	printf("\n");
}

void print_device_video_inputs(struct videohub_data *d) {
	unsigned int i, r, len = 68;
	if (!d->device.num_video_inputs)
		return;
	printf("Video inputs\n");
	printf_line(len);
	printf("  | ## | %-24s | n | %-24s | s |\n", "Video input name", "Routed to output");
	printf_line(len);
	for(i = 0; i < d->device.num_video_inputs; i++) {
		unsigned int num_outputs = 0, routed_to = 0;
		for(r = 0; r < d->device.num_video_outputs; r++) {
			if (d->outputs[r].routed_to == i) {
				num_outputs++;
				if (num_outputs == 1)
					routed_to = r; // The first output
			}
		}
		printf("  | %2d | %-24s | %d | ", i + 1, d->inputs[i].name,
			 num_outputs);
		if (num_outputs == 0) {
			printf("%-24s | %c |\n", "-", format_status(d->inputs[i].status));
		} else {
			printf("%-24s | %c |\n", d->outputs[routed_to].name, format_status(d->inputs[i].status));
			bool first_skipped = false;
			for(r = 0; r < d->device.num_video_outputs; r++) {
				if (d->outputs[r].routed_to == i) {
					if (!first_skipped) {
						first_skipped = true;
						continue;
					}
					printf("  | %2s | %-24s | %s | %-24s | %c |\n",
						" ", " ", " ", d->outputs[r].name, ' ');
				}
			}
		}
	}
	printf_line(len);
	printf("\n");
}

void print_device_video_outputs(struct videohub_data *d) {
	unsigned int i, len = 68;
	if (!d->device.num_video_outputs)
		return;
	printf("Video outputs\n");
	printf_line(len);
	printf("  | ## | x | %-24s | %-24s | s |\n", "Video output name", "Connected video input");
	printf_line(len);
	for(i = 0; i < d->device.num_video_outputs; i++) {
		printf("  | %2d | %c | %-24s | %-24s | %c |\n",
			i + 1,
			d->outputs[i].locked ? (d->outputs[i].locked_other ? 'L' : 'O') : ' ',
			d->outputs[i].name,
			d->inputs[d->outputs[i].routed_to].name,
			format_status(d->outputs[i].status)
		);
	}
	printf_line(len);
	printf("\n");
}

void print_device_backup(struct videohub_data *d) {
	unsigned int i;
	printf("videohubctrl \\\n");
	for(i = 0; i < d->device.num_video_inputs; i++)
		printf("  --vi-name %2d \"%s\" \\\n", i + 1, d->inputs[i].name);
	for(i = 0; i < d->device.num_video_outputs; i++)
		printf("  --vo-name %2d \"%s\" \\\n", i + 1, d->outputs[i].name);
	for(i = 0; i < d->device.num_video_outputs; i++)
		printf("  --vo-route %2d %2d \\\n", i + 1, d->outputs[i].routed_to + 1);
	for(i = 0; i < d->device.num_video_outputs; i++) {
		if (d->outputs[i].locked) {
			printf("  --vo-unlock %2d --vo-lock %2d%s\n", i + 1, i + 1,
				i + 1 < d->device.num_video_outputs ? " \\" : "");
		} else {
			printf("  --vo-unlock %2d%s\n", i + 1,
				i + 1 < d->device.num_video_outputs ? " \\" : "");
		}
	}
	printf("\n");
}
