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

#ifndef DISPLAY_H
#define DISPLAY_H

void print_device_info(struct videohub_data *d);
void print_device_video_inputs(struct videohub_data *d);
void print_device_video_outputs(struct videohub_data *d);
void print_device_monitoring_outputs(struct videohub_data *d);
void print_device_serial_ports(struct videohub_data *d);

void print_device_backup(struct videohub_data *d);

#endif
