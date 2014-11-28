/*
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
#include <getopt.h>
#include <unistd.h>

#include "data.h"
#include "cmd.h"
#include "net.h"
#include "util.h"
#include "display.h"
#include "version.h"

#include "libfuncs/libfuncs.h"

int debug;
int quiet;

static struct videohub_data maindata;
static int show_info = 1;
static int show_monitor = 0;
static int show_backup = 0;
static int show_list = 0;

enum list_actions {
	action_list_device		= (1 << 0),
	action_list_vinputs		= (1 << 1),
	action_list_voutputs	= (1 << 2),
};

static const char *program_id = PROGRAM_NAME " Version: " VERSION " Git: " GIT_VER;

static const char short_options[] = "h:p:qdHVimb";

static const struct option long_options[] = {
	{ "host",				required_argument, NULL, 'h' },
	{ "port",				required_argument, NULL, 'p' },
	{ "quiet",				no_argument,       NULL, 'q' },
	{ "debug",				no_argument,       NULL, 'd' },
	{ "help",				no_argument,       NULL, 'H' },
	{ "version",			no_argument,       NULL, 'V' },
	{ "info",				no_argument,       NULL, 'i' },
	{ "monitor",			no_argument,       NULL, 'm' },
	{ "backup",				no_argument,       NULL, 'b' },
	{ "list-device",		no_argument,       NULL, 901 },
	{ "list-vinputs",		no_argument,       NULL, 902 },
	{ "list-voutputs",		no_argument,       NULL, 903 },
	{ "vi-name",			required_argument, NULL, 1001 },
	{ "vo-name",			required_argument, NULL, 1002 },
	{ "vo-route",			required_argument, NULL, 1011 },
	{ "vo-lock",			required_argument, NULL, 1021 },
	{ "vo-unlock",			required_argument, NULL, 1022 },
	{ 0, 0, 0, 0 }
};

static void show_help(struct videohub_data *data) {
	printf("%s\n", program_id);
	printf("\n");
	printf(" Usage: " PROGRAM_NAME " --host <host> [..commands..]\n");
	printf("\n");
	printf("Main options:\n");
	printf(" -h --host <hostname>       | Set device hostname.\n");
	printf(" -p --port <port_number>    | Set device port (default: 9990).\n");
	printf("\n");
	printf("Misc options:\n");
	printf(" -d --debug                 | Increase logging verbosity.\n");
	printf(" -q --quiet                 | Suppress warnings.\n");
	printf(" -H --help                  | Show help screen.\n");
	printf(" -V --version               | Show program version.\n");
	printf("\n");
	printf("Commands:\n");
	printf(" -i --info                  | Show full device info (default command).\n");
	printf("                            . This command is shows the equivallent of\n");
	printf("                            .  running all --list-XXX commands.\n");
	printf(" -m --monitor               | Display real-time config changes monitor.\n");
	printf(" -b --backup                | Show the command line that will restore\n");
	printf("                            . the device to the current configuration.\n");
	printf("\n");
	printf(" --list-device              | Display device info.\n");
	printf(" --list-vinputs             | List device video inputs.\n");
	printf(" --list-voutputs            | List device video outputs.\n");
	printf("\n");
	printf("Video input/output configuration:\n");
	printf(" --vi-name <in_X> <name>    | Set video input port X name.\n");
	printf(" --vo-name <out_X> <name>   | Set video output port X name.\n");
	printf("\n");
	printf(" --vo-route <out_X> <in_Y>  | Connect video output X to video input Y\n");
	printf("\n");
	printf(" --vo-lock <out_X>          | Lock output port X.\n");
	printf(" --vo-unlock <out_X>        | Unlock output port X.\n");
	printf("\n");
	printf("  NOTE: For <in_X/out_X/in_Y> you may use port number or port name.\n");
	printf("\n");
}

static int num_parsed_cmds = 0;
static struct run_cmds {
	struct vcmd_entry	entry[MAX_RUN_CMDS];
} parsed_cmds;

static void parse_options(struct videohub_data *data, int argc, char **argv) {
	int j, err = 0;
	struct vcmd_entry *c = &parsed_cmds.entry[0];
	// Check environment
	data->dev_host = getenv("VIDEOHUB_HOST");
	data->dev_port = getenv("VIDEOHUB_PORT");
	// Set defaults
	if (!data->dev_port)
		data->dev_port = "9990";
	while ((j = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
		if (j == '?') // Invalid parameter
			exit(EXIT_FAILURE);
		switch (j) {
			case 'h': // --host
				data->dev_host = optarg;
				break;
			case 'p': // --port
				data->dev_port = optarg;
				break;
			case 'd': // --debug
				debug++;
				if (debug)
					quiet = 0; // Disable quiet
				break;
			case 'q': // --quiet
				quiet = !quiet;
				break;
			case 'i': // --info
				show_info = 1;
				break;
			case 'm': // --monitor
				show_monitor = 1;
				break;
			case 'b': // --backup
				show_backup = 1;
				break;
			case 901: show_list |= action_list_device; break; // --list-device
			case 902: show_list |= action_list_vinputs; break; // --list-vinputs
			case 903: show_list |= action_list_voutputs; break; // --list-voutputs
			case 1001: // --vi-name
			case 1002: // --vo-name
			case 1011: // --vi-route
			case 1012: // --vo-route
				if (num_parsed_cmds == ARRAY_SIZE(parsed_cmds.entry))
					die("No more than %u commands are supported.", num_parsed_cmds);
				if (optind == argc || argv[optind - 1][0] == '-' || argv[optind][0] == '-') {
					fprintf(stderr, "%s: option '%s' requires two arguments\n", argv[0], argv[optind - 2]);
					exit(EXIT_FAILURE);
				}
				switch (j) {
				case 1001: c->cmd = CMD_INPUT_LABELS; break; // --vi-name
				case 1002: c->cmd = CMD_OUTPUT_LABELS; break; // --vo-name
				case 1011: c->cmd = CMD_VIDEO_OUTPUT_ROUTING; break; // --vo-route
				}
				c->param1 = argv[optind - 1];
				c->param2 = argv[optind];
				c->param1 = argv[optind - 1];
				c->param2 = argv[optind];
				c = &parsed_cmds.entry[++num_parsed_cmds];
				break;
			case 1021: // --vo-lock
			case 1022: // --vo-unlock
				if (num_parsed_cmds == ARRAY_SIZE(parsed_cmds.entry))
					die("No more than %u commands are supported.", num_parsed_cmds);
				c->cmd = CMD_VIDEO_OUTPUT_LOCKS;
				c->param1 = argv[optind - 1];
				c->do_lock = (j == 1021);
				c = &parsed_cmds.entry[++num_parsed_cmds];
				break;
			case 'H': // --help
				show_help(data);
				exit(EXIT_SUCCESS);
			case 'V': // --version
				printf("%s\n", program_id);
				exit(EXIT_SUCCESS);
		}
	}

	if (!data->dev_host || !strtoul(data->dev_port, NULL, 10))
		err = 1;

	if (err) {
		show_help(data);
		if (!data->dev_host)
			fprintf(stderr, "ERROR: host is not set. Use --host option.\n");
		exit(EXIT_FAILURE);
	}

	d("Device address: %s:%s\n", data->dev_host, data->dev_port);
}

static int read_device_command_stream(struct videohub_data *d) {
	int ret, ncommands = 0;
	char buf[8192 + 1];
	memset(buf, 0, sizeof(buf));
	while ((ret = fdread_ex(d->dev_fd, buf, sizeof(buf) - 1, 5, 0, 0)) >= 0) {
		ncommands += parse_text_buffer(d, buf);
		memset(buf, 0, sizeof(buf));
	}
	return ncommands;
}

int main(int argc, char **argv) {
	struct videohub_data *data = &maindata;

	parse_options(data, argc, argv);
	set_log_io_errors(0);

	data->dev_fd = connect_client(SOCK_STREAM, data->dev_host, data->dev_port);
	if (data->dev_fd < 0)
		exit(EXIT_FAILURE);

	read_device_command_stream(data);

	if (!strlen(data->device.protocol_ver) || !strlen(data->device.model_name))
		die("The device does not respond correctly. Is it Videohub?");

	if (strstr(data->device.protocol_ver, "2.") != data->device.protocol_ver)
		die("Device protocol is %s but this program supports 2.x only.\n",
			data->device.protocol_ver);

	if (!data->device.dev_present) {
		if (data->device.needs_fw_update) {
			die("Device reports that it needs firmware update.");
		}
		die("Device reports that it's not present.");
	}

	if (data->device.num_video_inputs > ARRAY_SIZE(data->inputs))
		die("The device supports %d inputs. Recompile the program with more MAX_INPUTS (currently %d)",
			data->device.num_video_inputs, MAX_INPUTS);

	if (data->device.num_video_outputs > ARRAY_SIZE(data->outputs))
		die("The device supports %d outputs. Recompile the program with more MAX_OUTPUTS (currently %d)\n",
			data->device.num_video_outputs, MAX_OUTPUTS);

	if (num_parsed_cmds) {
		unsigned int i;
		for (i = 0; i < ARRAY_SIZE(parsed_cmds.entry); i++) {
			struct vcmd_entry *ve = &parsed_cmds.entry[i];
			if (!ve->param1)
				continue;
			prepare_cmd_entry(data, &parsed_cmds.entry[i]);
		}

		//print_device_settings(data);
		for (i = 0; i < ARRAY_SIZE(parsed_cmds.entry); i++) {
			char cmd_buffer[1024];
			struct vcmd_entry *ve = &parsed_cmds.entry[i];
			if (!ve->param1)
				continue;
			format_cmd_text(ve, cmd_buffer, sizeof(cmd_buffer));
			if (strlen(cmd_buffer)) {
				printf("%s", cmd_buffer);
				fdwrite(data->dev_fd, cmd_buffer, strlen(cmd_buffer));
			}
		}
		//usleep(100000);
		//read_device_command_stream(data);
		//print_device_settings(data);
	} else if (show_monitor) {
		while (1) {
			int sleeps = 0;
			printf("\e[2J\e[H"); // Clear screen
			time_t now = time(NULL);
			struct tm *tm = localtime(&now);
			printf("Last update: %s\n", asctime(tm));
			print_device_info(data);
			print_device_video_inputs(data);
			print_device_video_outputs(data);
			fflush(stdout);
			do {
				usleep(500000);
				if (++sleeps >= 20) {
					char *ping_cmd = "PING:\n\n";
					fdwrite(data->dev_fd, ping_cmd, strlen(ping_cmd));
				}
			} while (read_device_command_stream(data) == 0);
		}
	} else if (show_list) {
		if (show_list & action_list_device)		print_device_info(data);
		if (show_list & action_list_vinputs)	print_device_video_inputs(data);
		if (show_list & action_list_voutputs)	print_device_video_outputs(data);
		fflush(stdout);
	} else if (show_backup) {
		print_device_backup(data);
	} else if (show_info) {
		print_device_info(data);
		print_device_video_inputs(data);
		print_device_video_outputs(data);
		fflush(stdout);
	}

	shutdown_fd(&data->dev_fd);

	return 0;
}
