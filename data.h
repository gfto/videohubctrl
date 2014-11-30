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

#define MAX_PORTS 288
#define MAX_NAME_LEN 32
#define MAX_RUN_CMDS (288 * 5)

struct device_desc {
	bool			dev_present;
	bool			needs_fw_update;
	char			protocol_ver[16];
	char			model_name[MAX_NAME_LEN];
	char			unique_id[MAX_NAME_LEN];
	unsigned int	num_video_processing_units;
	unsigned int	num_serial_ports;
};

enum port_lock {
	PORT_UNLOCKED,
	PORT_LOCKED,
	PORT_LOCKED_OTHER,
};

struct port {
	char			name[MAX_NAME_LEN];
	// Port statuses are supported only by Universal Videohub
	// The statuses (actually they are connection types) are:
	//    BNC, Optical or None /missing port/
	char			status[8];
	unsigned int	routed_to;
	enum port_lock	lock;
};

struct port_set {
	unsigned int	num;
	struct port		port[MAX_PORTS];
};

struct videohub_data {
	char					*dev_host;
	char					*dev_port;
	int						dev_fd;
	struct device_desc		device;
	struct port_set			inputs;
	struct port_set			outputs;
	struct port_set			mon_outputs;
};

extern int debug;
extern int quiet;

#define d(fmt, arguments...) \
	do { \
		if (debug) \
			printf("debug: " fmt, ## arguments); \
	} while(0)

#define q(fmt, arguments...) \
	do { \
		if (!quiet) \
			fprintf(stderr, fmt, ## arguments); \
	} while(0)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))

#endif
