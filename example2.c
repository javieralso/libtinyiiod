/*
 * libtinyiiod - Tiny IIO Daemon Library
 *
 * Copyright (C) 2016 Analog Devices, Inc.
 * Author: Paul Cercueil <paul.cercueil@analog.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "iio.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/* List of devices to register */
static struct device_list *dev_list;

/*
 * At the moment, the library only supports static allocation.
 */
static struct device adc;
static struct channel voltage0, voltage1;
static struct attribute scale, raw;
static struct attribute sample_rate, direct_reg_access, length_align_bytes;

static ssize_t read_data(char *buf, size_t len)
{
	return fread(buf, 1, len, stdin);
}

static ssize_t write_data(const char *buf, size_t len)
{
	return fwrite(buf, 1, len, stdout);
}

static ssize_t read_dev_attr(void *data, const char *attr,
			 char *buf, size_t len, enum iio_attr_type type)
{
	switch (type) {
	case IIO_ATTR_TYPE_DEVICE:
		return (ssize_t) snprintf(buf, len, "1000");
	case IIO_ATTR_TYPE_DEBUG:
		return (ssize_t) snprintf(buf, len, "0");
	case IIO_ATTR_TYPE_BUFFER:
		return (ssize_t) snprintf(buf, len, "8");
	default:
		/* This should never happen */
		return -ENOENT;
	}
}

static ssize_t read_ch_attr(void *data, const char *channel,
			    bool ch_out, const char *attr, char *buf, ssize_t len)
{
	if (ch_out)
		return -ENOENT;

	if (!strcmp(channel, "voltage0")) {
		if (!strcmp(attr, "scale"))
			return (ssize_t) snprintf(buf, len, "0.033");
		else if (!strcmp(attr, "raw"))
			return (ssize_t) snprintf(buf, len, "256");
	} else if (!strcmp(channel, "voltage1")) {
		if (!strcmp(attr, "scale"))
			return (ssize_t) snprintf(buf, len, "0.033");
		else if (!strcmp(attr, "raw"))
			return (ssize_t) snprintf(buf, len, "128");
	}

	return -ENOENT;
}

static struct attr_accessors attr_accessors = {
	.read_attr = read_dev_attr,
	.write_attr = NULL,
};

static struct chn_accessors chn_accessors = {
	.read_attr = read_ch_attr,
	.write_attr = NULL,
	.read_data = NULL,
	.write_data = NULL,
};

static bool stop;

static void set_handler(int32_t signal_nb, void (*handler)(int32_t))
{
	struct sigaction sig;
	sigaction(signal_nb, NULL, &sig);
	sig.sa_handler = handler;
	sigaction(signal_nb, &sig, NULL);
}

static void quit_all(int32_t sig)
{
	stop = true;
}

int32_t main(void)
{
	struct context *context;

	/* Create the device */
	iio_new_static_device(&adc, "adc", 0, NULL, &attr_accessors, &chn_accessors);

	/* Device attributes */
	iio_new_static_attribute(&sample_rate, "sample_rate");
	iio_new_static_attribute(&direct_reg_access, "direct_reg_access");
	iio_new_static_attribute(&length_align_bytes, "lenght_align_bytes");

	/* Register the attributes to the device */
	iio_register_attribute(&sample_rate, &(IIO_ATTR_LIST((&adc))));
	iio_register_attribute(&direct_reg_access, &(IIO_ATTR_LIST((&adc))));
	iio_register_attribute(&length_align_bytes, &(IIO_ATTR_LIST((&adc))));

    /* Create common attributes for all the channels */
	iio_new_static_attribute(&scale, "scale");
	iio_new_static_attribute(&raw, "raw");

	/* Create channel 0 along with its attributes */
	iio_new_static_channel(&voltage0, "voltage0", "input");

	/* Register the attributes to its channel */
	iio_register_attribute(&scale, &(IIO_ATTR_LIST((&voltage0))));	
	iio_register_attribute(&raw, &(IIO_ATTR_LIST((&voltage0))));	

	/* Create channel 1 along with its attributes */
	iio_new_static_channel(&voltage1, "voltage1", "input");

	/* Register the attributes to its channel */
	iio_register_attribute(&scale, &(IIO_ATTR_LIST((&voltage1))));	
	iio_register_attribute(&raw, &(IIO_ATTR_LIST((&voltage1))));

	/* Register the channels within the device */
	iio_register_channel(&voltage0, &(IIO_CHANNEL_LIST((&adc))));	
	iio_register_channel(&voltage1, &(IIO_CHANNEL_LIST((&adc))));	

	/* Add the device to the list of devices to register within the context */
	iio_register_device(&adc, &dev_list);

	context = iio_init("tiny", "Tiny IIOD", dev_list, write_data, read_data);

	assert(context != NULL);

	set_handler(SIGHUP, &quit_all);
	set_handler(SIGPIPE, &quit_all);
	set_handler(SIGINT, &quit_all);
	set_handler(SIGSEGV, &quit_all);
	set_handler(SIGTERM, &quit_all);

	while (!stop)
		iio_read_command(context);
	
	/* FIXME: The library does not support destruction of elements */
	tinyiiod_destroy(context->iiod);

	return 0;
}
