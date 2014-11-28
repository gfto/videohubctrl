/*
 * === Main data structures and helpers ===
 *
 * Blackmagic Design Videohub control application
 * Copyright (C) 2014 Unix Solutions Ltd.
 * Written by Georgi Chorbadzhiyski
 *
 * Released under MIT license.
 * See LICENSE-MIT.txt for license terms.
 *
 */

#ifndef DATA_H
#define DATA_H

#include <stdbool.h>

#define MAX_INPUTS 288
#define MAX_OUTPUTS 288
#define MAX_NAME_LEN 64

#define MAX_RUN_CMDS (MAX_INPUTS + (MAX_OUTPUTS * 2))

struct device_desc {
	bool			dev_present;
	bool			needs_fw_update;
	char			protocol_ver[16];
	char			model_name[MAX_NAME_LEN];
	char			unique_id[MAX_NAME_LEN];
	unsigned int	num_video_inputs;
	unsigned int	num_video_processing_units;
	unsigned int	num_video_outputs;
	unsigned int	num_video_monitoring_outputs;
	unsigned int	num_serial_ports;
};

struct input_desc {
	char			name[MAX_NAME_LEN];
};

struct output_desc {
	char			name[MAX_NAME_LEN];
	unsigned int	routed_to;
	bool			locked;
	bool			locked_other;
};

struct videohub_data {
	char					*dev_host;
	char					*dev_port;
	int						dev_fd;
	struct device_desc		device;
	struct input_desc		inputs[MAX_INPUTS];
	struct output_desc		outputs[MAX_OUTPUTS];
};

extern int debug;
extern int quiet;

#define d(fmt, arguments...) \
	do { \
		if (debug) \
			printf("debug: " fmt, ## arguments); \
	} while(0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))

#endif
