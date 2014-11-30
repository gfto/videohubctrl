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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

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
static char *test_data;

enum list_actions {
	action_list_device		= (1 << 0),
	action_list_vinputs		= (1 << 1),
	action_list_voutputs	= (1 << 2),
	action_list_moutputs	= (1 << 3),
};

static const char *program_id = PROGRAM_NAME " Version: " VERSION " Git: " GIT_VER;

static const char short_options[] = "h:p:T:qdHVimb";

static const struct option long_options[] = {
	{ "host",				required_argument, NULL, 'h' },
	{ "port",				required_argument, NULL, 'p' },
	{ "test-input",			required_argument, NULL, 'T' },
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
	{ "list-moutputs",		no_argument,       NULL, 904 },
	{ "vi-name",			required_argument, NULL, 1001 },
	{ "vo-name",			required_argument, NULL, 2001 },
	{ "vo-input",			required_argument, NULL, 2002 },
	{ "vo-route",			required_argument, NULL, 2002 }, // Alias of --vo-input
	{ "vo-lock",			required_argument, NULL, 2003 },
	{ "vo-unlock",			required_argument, NULL, 2004 },
	{ "mo-name",			required_argument, NULL, 3001 },
	{ "mo-input",			required_argument, NULL, 3002 },
	{ "mo-route",			required_argument, NULL, 3002 }, // Alias of --mo-input
	{ "mo-lock",			required_argument, NULL, 3003 },
	{ "mo-unlock",			required_argument, NULL, 3004 },
	{ 0, 0, 0, 0 }
};

static void show_help(struct videohub_data *data) {
	printf("%s\n", program_id);
	printf("\n");
	printf(" Usage: " PROGRAM_NAME " --host <host> [..commands..]\n");
	printf("\n");
	printf("Main options:\n");
	printf(" -h --host <host>           | Set device host name.\n");
	printf(" -p --port <port_number>    | Set device port (default: 9990).\n");
	printf("\n");
	printf("Commands:\n");
	printf(" -i --info                  | Show full device info (default command).\n");
	printf("                            . This command is shows the equivalent of\n");
	printf("                            .  running all --list-XXX commands.\n");
	printf(" -m --monitor               | Display real-time config changes monitor.\n");
	printf(" -b --backup                | Show the command line that will restore\n");
	printf("                            . the device to the current configuration.\n");
	printf("\n");
	printf(" --list-device              | Display device info.\n");
	printf(" --list-vinputs             | List device video inputs.\n");
	printf(" --list-voutputs            | List device video outputs.\n");
	printf(" --list-moutputs            | List device monitoring outputs.\n");
	printf("\n");
	printf("Video inputs configuration:\n");
	printf(" --vi-name <in_X> <name>    | Set video input port X name.\n");
	printf("\n");
	printf("Video outputs configuration:\n");
	printf(" --vo-name <out_X> <name>   | Set video output port X name.\n");
	printf(" --vo-input <out_X> <in_Y>  | Connect video output X to video input Y\n");
	printf(" --vo-lock <out_X>          | Lock output port X.\n");
	printf(" --vo-unlock <out_X>        | Unlock output port X.\n");
	printf("\n");
	printf("Monitoring outputs configuration:\n");
	printf(" --mo-name <mout_X> <name>  | Set monitoring port X name.\n");
	printf(" --mo-route <mout_X> <in_Y> | Connect monitoring X to video input Y\n");
	printf(" --mo-lock <mout_X>         | Lock monitoring port X.\n");
	printf(" --mo-unlock <mout_X>       | Unlock monitoring port X.\n");
	printf("\n");
	printf("Misc options:\n");
	printf(" -T --test-input <file>     | Read commands from <file>.\n");
	printf(" -d --debug                 | Increase logging verbosity.\n");
	printf(" -q --quiet                 | Suppress warnings.\n");
	printf(" -H --help                  | Show help screen.\n");
	printf(" -V --version               | Show program version.\n");
	printf("\n");
}

static int num_parsed_cmds = 0;
static struct run_cmds {
	struct vcmd_entry	entry[MAX_RUN_CMDS];
} parsed_cmds;

static void parse_cmd2(int argc, char **argv, enum vcmd vcmd) {
	struct vcmd_entry *c = &parsed_cmds.entry[num_parsed_cmds];
	if (num_parsed_cmds == ARRAY_SIZE(parsed_cmds.entry))
		die("No more than %u commands are supported.", num_parsed_cmds);
	if (optind == argc || argv[optind - 1][0] == '-' || argv[optind][0] == '-') {
		fprintf(stderr, "%s: option '%s' requires two arguments\n", argv[0], argv[optind - 2]);
		exit(EXIT_FAILURE);
	}
	c->cmd = &videohub_commands[vcmd];
	c->param1 = argv[optind - 1];
	c->param2 = argv[optind];
	num_parsed_cmds++;
}

static void parse_cmd1(int argc, char **argv, enum vcmd vcmd, bool do_lock) {
	struct vcmd_entry *c = &parsed_cmds.entry[num_parsed_cmds];
	if (num_parsed_cmds == ARRAY_SIZE(parsed_cmds.entry))
		die("No more than %u commands are supported.", num_parsed_cmds);
	c->cmd = &videohub_commands[vcmd];
	c->param1 = argv[optind - 1];
	c->do_lock = do_lock;
	num_parsed_cmds++;
}

static void parse_options(struct videohub_data *data, int argc, char **argv) {
	int j, err = 0;
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
			case 'T': { // --test-input
				struct stat st;
				FILE *f;
				if (stat(optarg, &st) != 0)
					die("Can't stat %s: %s", optarg, strerror(errno));
				f = fopen(optarg, "r");
				if (!f)
					die("Can't open %s: %s", optarg, strerror(errno));
				test_data = xzalloc(st.st_size);
				if (fread(test_data, st.st_size, 1, f) < 1)
					die("Can't read from %s: %s", optarg, strerror(errno));
				fclose(f);
				data->dev_host = "sdi-matrix";
				break;
			}
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
			case 904: show_list |= action_list_moutputs; break; // --list-moutputs
			case 1001: parse_cmd2(argc, argv, CMD_INPUT_LABELS); break; // --vi-name
			case 2001: parse_cmd2(argc, argv, CMD_OUTPUT_LABELS); break; // --vo-name
			case 2002: parse_cmd2(argc, argv, CMD_VIDEO_OUTPUT_ROUTING); break; // --vo-input
			case 2003: parse_cmd1(argc, argv, CMD_VIDEO_OUTPUT_LOCKS, true); break; // --vo-lock
			case 2004: parse_cmd1(argc, argv, CMD_VIDEO_OUTPUT_LOCKS, false); break; // --vo-unlock
			case 3001: parse_cmd2(argc, argv, CMD_MONITORING_OUTPUT_LABELS); break; // --mo-name
			case 3002: parse_cmd2(argc, argv, CMD_MONITORING_OUTPUT_ROUTING); break; // --mo-route
			case 3003: parse_cmd1(argc, argv, CMD_MONITORING_OUTPUT_LOCKS, true); break; // --mo-lock
			case 3004: parse_cmd1(argc, argv, CMD_MONITORING_OUTPUT_LOCKS, false); break; // --mo-unlock
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

static void check_number_of_ports(struct port_set *p) {
	if (p->num > ARRAY_SIZE(p->port))
		die("The device supports %d ports. Increase MAX_PORTS (%lu) and recompile the program.",
			p->num, ARRAY_SIZE(p->port));
}

static void print_device_full(struct videohub_data *d) {
	print_device_info(d);
	print_device_video_inputs(d);
	print_device_video_outputs(d);
	print_device_monitoring_outputs(d);
	fflush(stdout);
}

static int read_device_command_stream(struct videohub_data *d) {
	int ret, ncommands = 0;
	char buf[8192 + 1];
	if (test_data)
		return 0;
	memset(buf, 0, sizeof(buf));
	while ((ret = fdread_ex(d->dev_fd, buf, sizeof(buf) - 1, 5, 0, 0)) >= 0) {
		ncommands += parse_text_buffer(d, buf);
		memset(buf, 0, sizeof(buf));
	}
	return ncommands;
}

static void send_device_command(struct videohub_data *d, char *cmd_buffer) {
	if (!test_data)
		fdwrite(d->dev_fd, cmd_buffer, strlen(cmd_buffer));
	else
		parse_text_buffer(d, cmd_buffer);
}

int main(int argc, char **argv) {
	struct videohub_data *data = &maindata;

	parse_options(data, argc, argv);
	set_log_io_errors(0);

	if (!test_data) {
		data->dev_fd = connect_client(SOCK_STREAM, data->dev_host, data->dev_port);
		if (data->dev_fd < 0)
			exit(EXIT_FAILURE);
	}

	read_device_command_stream(data);

	if (test_data)
		parse_text_buffer(data, test_data);

	if (!strlen(data->device.protocol_ver) || !strlen(data->device.model_name))
		die("The device does not respond correctly. Is it Videohub?");

	if (strstr(data->device.protocol_ver, "2.") != data->device.protocol_ver)
		die("Device protocol is %s but this program supports 2.x only.\n",
			data->device.protocol_ver);

	if (!data->device.dev_present) {
		if (data->device.needs_fw_update) {
			die("Device reports that it needs firmware update.");
		}
		die("Device reports that it is not present.");
	}

	check_number_of_ports(&data->inputs);
	check_number_of_ports(&data->outputs);

	if (num_parsed_cmds) {
		unsigned int i;
		for (i = 0; i < ARRAY_SIZE(parsed_cmds.entry); i++) {
			struct vcmd_entry *ve = &parsed_cmds.entry[i];
			if (!ve->param1)
				continue;
			prepare_cmd_entry(data, &parsed_cmds.entry[i]);
		}

		for (i = 0; i < ARRAY_SIZE(parsed_cmds.entry); i++) {
			char cmd_buffer[1024];
			struct vcmd_entry *ve = &parsed_cmds.entry[i];
			if (!ve->param1)
				continue;
			format_cmd_text(ve, cmd_buffer, sizeof(cmd_buffer));
			if (strlen(cmd_buffer)) {
				show_cmd(data, ve);
				send_device_command(data, cmd_buffer);
				read_device_command_stream(data);
			}
		}
		// Show the result after commands
		if (test_data)
			print_device_full(data);
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
				if (++sleeps >= 20)
					send_device_command(data, "PING:\n\n");
			} while (read_device_command_stream(data) == 0);
		}
	} else if (show_list) {
		if (show_list & action_list_device)		print_device_info(data);
		if (show_list & action_list_vinputs)	print_device_video_inputs(data);
		if (show_list & action_list_voutputs)	print_device_video_outputs(data);
		if (show_list & action_list_moutputs)	print_device_monitoring_outputs(data);
		fflush(stdout);
	} else if (show_backup) {
		print_device_backup(data);
	} else if (show_info) {
		print_device_full(data);
	}

	shutdown_fd(&data->dev_fd);
	free(test_data);

	return 0;
}
