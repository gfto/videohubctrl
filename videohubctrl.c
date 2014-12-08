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
	action_list_serial		= (1 << 4),
	action_list_proc_units	= (1 << 5),
	action_list_frames		= (1 << 6),
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
	{ "list-inputs",		no_argument,       NULL, 902 },
	{ "list-vinputs",		no_argument,       NULL, 902 },
	{ "list-outputs",		no_argument,       NULL, 903 },
	{ "list-voutputs",		no_argument,       NULL, 903 },
	{ "list-monitoring",	no_argument,       NULL, 904 },
	{ "list-moutputs",		no_argument,       NULL, 904 },
	{ "list-serial",		no_argument,       NULL, 905 },
	{ "list-proc-units",	no_argument,       NULL, 906 },
	{ "list-frames",		no_argument,       NULL, 907 },

	{ "set-name",			required_argument, NULL, 950 },

	{ "in-name",			required_argument, NULL, 1001 },
	{ "in-output",			required_argument, NULL, 1002 },
	{ "in-monitor",			required_argument, NULL, 1003 },
	{ "vi-name",			required_argument, NULL, 1001 },
	{ "vi-output",			required_argument, NULL, 1002 },
	{ "vi-monitor",			required_argument, NULL, 1003 },

	{ "out-name",			required_argument, NULL, 2001 },
	{ "out-input",			required_argument, NULL, 2002 },
	{ "out-route",			required_argument, NULL, 2002 }, // Alias of --vo-input
	{ "out-lock",			required_argument, NULL, 2003 },
	{ "out-unlock",			required_argument, NULL, 2004 },
	{ "vo-name",			required_argument, NULL, 2001 },
	{ "vo-input",			required_argument, NULL, 2002 },
	{ "vo-route",			required_argument, NULL, 2002 }, // Alias of --vo-input
	{ "vo-lock",			required_argument, NULL, 2003 },
	{ "vo-unlock",			required_argument, NULL, 2004 },

	{ "mon-name",			required_argument, NULL, 3001 },
	{ "mon-input",			required_argument, NULL, 3002 },
	{ "mon-route",			required_argument, NULL, 3002 }, // Alias of --mo-input
	{ "mon-lock",			required_argument, NULL, 3003 },
	{ "mon-unlock",			required_argument, NULL, 3004 },
	{ "mo-name",			required_argument, NULL, 3001 },
	{ "mo-input",			required_argument, NULL, 3002 },
	{ "mo-route",			required_argument, NULL, 3002 }, // Alias of --mo-input
	{ "mo-lock",			required_argument, NULL, 3003 },
	{ "mo-unlock",			required_argument, NULL, 3004 },

	{ "ser-name",			required_argument, NULL, 4001 },
	{ "ser-input",			required_argument, NULL, 4002 },
	{ "ser-connect",		required_argument, NULL, 4002 }, // Alias of --se-input
	{ "ser-route",			required_argument, NULL, 4002 }, // Alias of --se-input
	{ "ser-lock",			required_argument, NULL, 4003 },
	{ "ser-unlock",			required_argument, NULL, 4004 },
	{ "ser-dir",			required_argument, NULL, 4005 },
	{ "ser-clear",			required_argument, NULL, 4006 },
	{ "se-name",			required_argument, NULL, 4001 },
	{ "se-input",			required_argument, NULL, 4002 },
	{ "se-connect",			required_argument, NULL, 4002 }, // Alias of --se-input
	{ "se-route",			required_argument, NULL, 4002 }, // Alias of --se-input
	{ "se-lock",			required_argument, NULL, 4003 },
	{ "se-unlock",			required_argument, NULL, 4004 },
	{ "se-dir",				required_argument, NULL, 4005 },
	{ "se-clear",			required_argument, NULL, 4006 },

	{ "pu-input",			required_argument, NULL, 5001 },
	{ "pu-connect",			required_argument, NULL, 5001 }, // Alias of --pu-input
	{ "pu-route",			required_argument, NULL, 5001 }, // Alias of --pu-input
	{ "pu-lock",			required_argument, NULL, 5002 },
	{ "pu-unlock",			required_argument, NULL, 5003 },
	{ "pu-clear",			required_argument, NULL, 5004 },

	{ "fr-name",			required_argument, NULL, 6001 },
	{ "fr-output",			required_argument, NULL, 6002 },
	{ "fr-connect",			required_argument, NULL, 6002 }, // Alias of --fr-output
	{ "fr-input",			required_argument, NULL, 6002 }, // Alias of --fr-output
	{ "fr-route",			required_argument, NULL, 6002 }, // Alias of --fr-output
	{ "fr-lock",			required_argument, NULL, 6003 },
	{ "fr-unlock",			required_argument, NULL, 6004 },
	{ "fr-clear",			required_argument, NULL, 6006 },
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
	printf(" --list-inputs              | List device input ports.\n");
	printf(" --list-outputs             | List device output ports.\n");
	printf(" --list-monitor             | List device monitoring outputs.\n");
	printf(" --list-serial              | List device serial ports.\n");
	printf(" --list-proc-units          | List device processing units.\n");
	printf(" --list-frames              | List device frame buffers.\n");
	printf("\n");
	printf(" --set-name <name>          | Set the device \"friendly name\".\n");
	printf("\n");
	printf("Inputs configuration:\n");
	printf(" --in-name <in_X> <name>       | Set input port X name.\n");
	printf(" --in-output <in_X> <out_Y>    | Route input X to output Y.\n");
	printf(" --in-monitor <in_X> <mout_Y>  | Route input X to monitor port Y.\n");
	printf("\n");
	printf("Outputs configuration:\n");
	printf(" --out-name <out_X> <name>     | Set output port X name.\n");
	printf(" --out-input <out_X> <in_Y>    | Connect output X to input Y.\n");
	printf(" --out-lock <out_X>            | Lock output port X.\n");
	printf(" --out-unlock <out_X>          | Unlock output port X.\n");
	printf("\n");
	printf("Monitoring outputs configuration:\n");
	printf(" --mon-name <mout_X> <name>    | Set monitoring port X name.\n");
	printf(" --mon-input <mout_X> <in_Y>   | Connect monitoring X to input Y.\n");
	printf(" --mon-lock <mout_X>           | Lock monitoring port X.\n");
	printf(" --mon-unlock <mout_X>         | Unlock monitoring port X.\n");
	printf("\n");
	printf("Serial ports configuration:\n");
	printf(" --ser-name <ser_X> <name>     | Set serial port X name.\n");
	printf(" --ser-connect <ser_X> <ser_Y> | Connect serial X to serial Y.\n");
	printf(" --ser-clear <ser_X>           | Disconnect serial port X from serial Y.\n");
	printf(" --ser-lock <ser_X>            | Lock serial port X.\n");
	printf(" --ser-unlock <ser_X>          | Unlock serial port X.\n");
	printf(" --ser-dir <ser_X> <dir>       | Set serial port X direction.\n");
	printf("                               . <dir> can be 'auto', 'in' or 'out'.\n");
	printf("\n");
	printf("Processing units configuration:\n");
	printf(" --pu-input <pu_X> <in_Y>      | Connect processing unit X to input Y.\n");
	printf(" --pu-clear <pu_X>             | Disconnect unit X from input Y.\n");
	printf(" --pu-lock <pu_X>              | Lock processing unit X.\n");
	printf(" --pu-unlock <pu_X>            | Unlock processing unit X.\n");
	printf("\n");
	printf("Frames configuration:\n");
	printf(" --fr-name <fr_X> <name>       | Set frame X name.\n");
	printf(" --fr-output <fr_X> <out_Y>    | Output frame X to output Y.\n");
	printf(" --fr-clear <fr_X>             | Stop outputing frame X to the output.\n");
	printf(" --fr-lock <fr_X>              | Lock frame X.\n");
	printf(" --fr-unlock <rf_X>            | Unlock frame X.\n");
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

static void check_num_parsed_cmds(void) {
	if (num_parsed_cmds == ARRAY_SIZE(parsed_cmds.entry))
		die("No more than %u commands are supported.", num_parsed_cmds);
}

static void parse_cmd2(int argc, char **argv, enum vcmd vcmd) {
	check_num_parsed_cmds();
	struct vcmd_entry *c = &parsed_cmds.entry[num_parsed_cmds];
	if (optind == argc || argv[optind - 1][0] == '-' || argv[optind][0] == '-') {
		fprintf(stderr, "%s: option '%s' requires two arguments\n", argv[0], argv[optind - 2]);
		exit(EXIT_FAILURE);
	}
	c->cmd = &videohub_commands[vcmd];
	c->p1.param = argv[optind - 1];
	c->p2.param = argv[optind];
	if (vcmd == CMD_SERIAL_PORT_DIRECTIONS) {
		if (strcasecmp("in", c->p2.param) == 0)        c->direction = DIR_CONTROL;
		else if (strcasecmp("out", c->p2.param) == 0)  c->direction = DIR_SLAVE;
		else if (strcasecmp("auto", c->p2.param) == 0) c->direction = DIR_AUTO;
		else die("Invalid serial port direction '%s'. Allowed are: in, out, auto.", c->p2.param);
	}
	num_parsed_cmds++;
}

static void parse_cmd2s(int argc, char **argv, enum vcmd vcmd) {
	check_num_parsed_cmds();
	struct vcmd_entry *c = &parsed_cmds.entry[num_parsed_cmds];
	c->cmd = &videohub_commands[vcmd];
	c->p1.param = optarg;
	c->p2.param = "1"; // Fake
	c->clear_port = true;
	num_parsed_cmds++;
}

static void parse_cmd1(int argc, char **argv, enum vcmd vcmd, bool do_lock) {
	check_num_parsed_cmds();
	struct vcmd_entry *c = &parsed_cmds.entry[num_parsed_cmds];
	c->cmd = &videohub_commands[vcmd];
	c->p1.param = argv[optind - 1];
	c->do_lock = do_lock;
	num_parsed_cmds++;
}

static void set_device_option(char *setting, char *value) {
	check_num_parsed_cmds();
	struct vcmd_entry *c = &parsed_cmds.entry[num_parsed_cmds];
	c->cmd = &videohub_commands[CMD_VIDEOHUB_DEVICE];
	c->p1.param = setting;
	c->p2.param = value;
	num_parsed_cmds++;
}

static void switch_cmd_args(void) {
	char *p1, *p2;
	parsed_cmds.entry[num_parsed_cmds-1].reversed_args = 1;
	p1 = parsed_cmds.entry[num_parsed_cmds-1].p1.param;
	p2 = parsed_cmds.entry[num_parsed_cmds-1].p2.param;
	parsed_cmds.entry[num_parsed_cmds-1].p1.param = p2;
	parsed_cmds.entry[num_parsed_cmds-1].p2.param = p1;
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
				test_data = xzalloc(st.st_size + 1);
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
			case 905: show_list |= action_list_serial; break; // --list-serial
			case 906: show_list |= action_list_proc_units; break; // --list-proc-units
			case 907: show_list |= action_list_frames; break; // --list-frames
			case 950: set_device_option("Friendly name", optarg); break; // --set-name
			case 1001: parse_cmd2(argc, argv, CMD_INPUT_LABELS); break; // --vi-name
			case 1002: parse_cmd2(argc, argv, CMD_VIDEO_OUTPUT_ROUTING); switch_cmd_args(); break; // --vi-output
			case 1003: parse_cmd2(argc, argv, CMD_MONITORING_OUTPUT_ROUTING); switch_cmd_args(); break; // --vi-monitor
			case 2001: parse_cmd2(argc, argv, CMD_OUTPUT_LABELS); break; // --vo-name
			case 2002: parse_cmd2(argc, argv, CMD_VIDEO_OUTPUT_ROUTING); break; // --vo-input
			case 2003: parse_cmd1(argc, argv, CMD_VIDEO_OUTPUT_LOCKS, true); break; // --vo-lock
			case 2004: parse_cmd1(argc, argv, CMD_VIDEO_OUTPUT_LOCKS, false); break; // --vo-unlock
			case 3001: parse_cmd2(argc, argv, CMD_MONITORING_OUTPUT_LABELS); break; // --mo-name
			case 3002: parse_cmd2(argc, argv, CMD_MONITORING_OUTPUT_ROUTING); break; // --mo-input
			case 3003: parse_cmd1(argc, argv, CMD_MONITORING_OUTPUT_LOCKS, true); break; // --mo-lock
			case 3004: parse_cmd1(argc, argv, CMD_MONITORING_OUTPUT_LOCKS, false); break; // --mo-unlock
			case 4001: parse_cmd2(argc, argv, CMD_SERIAL_PORT_LABELS); break; // --se-name
			case 4002: parse_cmd2(argc, argv, CMD_SERIAL_PORT_ROUTING); break; // --se-input
			case 4003: parse_cmd1(argc, argv, CMD_SERIAL_PORT_LOCKS, true); break; // --se-lock
			case 4004: parse_cmd1(argc, argv, CMD_SERIAL_PORT_LOCKS, false); break; // --se-unlock
			case 4005: parse_cmd2(argc, argv, CMD_SERIAL_PORT_DIRECTIONS); break; // --se-dir
			case 4006: parse_cmd2s(argc, argv, CMD_SERIAL_PORT_ROUTING); break; // --se-clear
			case 5001: parse_cmd2(argc, argv, CMD_PROCESSING_UNIT_ROUTING); break; // --pu-input
			case 5002: parse_cmd1(argc, argv, CMD_PROCESSING_UNIT_LOCKS, true); break; // --pu-lock
			case 5003: parse_cmd1(argc, argv, CMD_PROCESSING_UNIT_LOCKS, false); break; // --pu-unlock
			case 5004: parse_cmd2s(argc, argv, CMD_PROCESSING_UNIT_ROUTING); break; // --pu-clear
			case 6001: parse_cmd2(argc, argv, CMD_FRAME_LABELS); break; // --fr-name
			case 6002: parse_cmd2(argc, argv, CMD_FRAME_BUFFER_ROUTING); break; // --fr-input
			case 6003: parse_cmd1(argc, argv, CMD_FRAME_BUFFER_LOCKS, true); break; // --fr-lock
			case 6004: parse_cmd1(argc, argv, CMD_FRAME_BUFFER_LOCKS, false); break; // --fr-unlock
			case 6005: parse_cmd2s(argc, argv, CMD_FRAME_BUFFER_ROUTING); break; // --fr-clear
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

static void reset_routed_to(struct port_set *p) {
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(p->port); i++)
		p->port[i].routed_to = NO_PORT;
}

static void print_device_full(struct videohub_data *d) {
	print_device_info(d);
	print_device_video_inputs(d);
	print_device_video_outputs(d);
	print_device_monitoring_outputs(d);
	print_device_serial_ports(d);
	print_device_processing_units(d);
	print_device_frame_buffers(d);
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

	reset_routed_to(&data->inputs);
	reset_routed_to(&data->outputs);
	reset_routed_to(&data->serial);
	reset_routed_to(&data->proc_units);
	reset_routed_to(&data->frames);
	read_device_command_stream(data);

	if (test_data)
		parse_text_buffer(data, test_data);

	if (!strlen(data->device.protocol_ver) || !strlen(data->device.model_name))
		die("The device does not respond correctly. Is it Videohub?");

	if (strstr(data->device.protocol_ver, "2.") != data->device.protocol_ver)
		q("WARNING: Device protocol is %s but this program is tested with 2.x only.\n",
			data->device.protocol_ver);

	if (!data->device.dev_present) {
		if (data->device.needs_fw_update) {
			die("Device reports that it needs firmware update.");
		}
		die("Device reports that it is not present.");
	}

	check_number_of_ports(&data->inputs);
	check_number_of_ports(&data->outputs);
	check_number_of_ports(&data->mon_outputs);
	check_number_of_ports(&data->serial);
	check_number_of_ports(&data->proc_units);
	check_number_of_ports(&data->frames);

	if (num_parsed_cmds) {
		unsigned int i;
		for (i = 0; i < ARRAY_SIZE(parsed_cmds.entry); i++) {
			struct vcmd_entry *ve = &parsed_cmds.entry[i];
			if (!ve->p1.param)
				continue;
			prepare_cmd_entry(data, &parsed_cmds.entry[i]);
		}

		for (i = 0; i < ARRAY_SIZE(parsed_cmds.entry); i++) {
			char cmd_buffer[1024];
			struct vcmd_entry *ve = &parsed_cmds.entry[i];
			if (!ve->p1.param)
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
		if (show_list & action_list_serial)		print_device_serial_ports(data);
		if (show_list & action_list_proc_units)	print_device_processing_units(data);
		if (show_list & action_list_frames)		print_device_frame_buffers(data);
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
