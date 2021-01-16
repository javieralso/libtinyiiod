/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, Javier Almansa Sobrino
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.

 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tinyiiod.h"
#include <iio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static const char * const dtd =
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<!DOCTYPE context ["
    "<!ELEMENT context (device)*>"
    "<!ELEMENT device (channel | attribute | debug-attribute | buffer-attribute)*>"
    "<!ELEMENT channel (scan-element?, attribute*)>"
    "<!ELEMENT attribute EMPTY>"
    "<!ELEMENT scan-element EMPTY>"
    "<!ELEMENT debug-attribute EMPTY>"
    "<!ELEMENT buffer-attribute EMPTY>"
    "<!ATTLIST context name CDATA #REQUIRED description CDATA #IMPLIED>"
    "<!ATTLIST device id CDATA #REQUIRED name CDATA #IMPLIED>"
    "<!ATTLIST channel id CDATA #REQUIRED type (input|output) #REQUIRED name CDATA #IMPLIED>"
    "<!ATTLIST scan-element index CDATA #REQUIRED format CDATA #REQUIRED scale CDATA #IMPLIED>"
    "<!ATTLIST attribute name CDATA #REQUIRED filename CDATA #IMPLIED>"
    "<!ATTLIST debug-attribute name CDATA #REQUIRED>"
    "<!ATTLIST buffer-attribute name CDATA #REQUIRED value CDATA #IMPLIED>]>";

static struct context context;

static ssize_t read_attr(const char *device, const char *attr,
			 char *buf, size_t len, enum iio_attr_type type)
{
    struct device *dev;
    struct attribute *attribute;
    struct device_list *dev_list;
    struct attribute_list *_attrs;
    struct attr_accessors *accessors;

    for_each_device(IIO_DEVICE_LIST((&context)), dev, dev_list) {
        /*
         * FIXME: atoi() does not detect errors, so we assume the input is
         * correct. Maybe this should be changed by strtol(), which is able
         * to detect errors.
         */
        if (atoi(device) == IIO_ID(dev)) {
            for_each_attribute(IIO_ATTR_LIST(dev), attribute, _attrs) {
                if(strcmp(IIO_NAME(attribute), attr) == 0) {
                    accessors = IIO_ATTR_ACCESSORS(dev);
                    if (accessors->read_attr == NULL) {
                        /* No accessor installed */
                        return -ENOSYS;
                    }
                    /* Run the accessor function */
                    return accessors->read_attr(IIO_DEV_DATA(dev), attr,
                                                buf, len, type);
                }
            }
            /* No matching attribute on device. */
            return -ENOENT;
        }
    }
    /* No matching device found. */
    return -ENOENT;
}

static ssize_t write_attr(const char *device, const char *attr,
			  const char *buf, size_t len, enum iio_attr_type type)
{
    struct device *dev;
    struct attribute *attribute;
    struct device_list *dev_list;
    struct attribute_list *_attrs;
    struct attr_accessors *accessors;

    for_each_device(IIO_DEVICE_LIST((&context)), dev, dev_list) {
        /*
         * FIXME: atoi() does not detect errors, so we assume the input is
         * correct. Maybe this should be changed by strtol(), which is able
         * to detect errors.
         */
        if (atoi(device) == IIO_ID(dev)) {
            for_each_attribute(IIO_ATTR_LIST(dev), attribute, _attrs) {
                if(strcmp(IIO_NAME(attribute), attr) == 0) {
                    accessors = IIO_ATTR_ACCESSORS(dev);
                    if (accessors->write_attr == NULL) {
                        /* No accessor installed */
                        return -ENOSYS;
                    }
                    /* Run the accessor function */
                    return accessors->write_attr(IIO_DEV_DATA(dev), attr,
                                                 buf, len, type);
                }
            }
            /* No matching attribute on device. */
            return -ENOENT;
        }
    }
    /* No matching device found. */
    return -ENOENT;
}

static ssize_t ch_read_attr(const char *device, const char *channel,
			    bool ch_out, const char *attr, char *buf, size_t len)
{
    struct device *dev;
    struct channel *chn;
    struct attribute *attribute;
    struct device_list *dev_list;
    struct channel_list *chn_list;
    struct attribute_list *attr_list;
    struct chn_accessors *accessors;

    for_each_device(IIO_DEVICE_LIST((&context)), dev, dev_list) {
        /*
         * FIXME: atoi() does not detect errors, so we assume the input is
         * correct. Maybe this should be changed by strtol(), which is able
         * to detect errors.
         */
        if (atoi(device) == IIO_ID(dev)) {
            for_each_channel(IIO_CHANNEL_LIST(dev), chn, chn_list) {
                if (strcmp(IIO_ID(chn), channel) == 0) {
                    for_each_attribute(IIO_ATTR_LIST(chn), attribute, attr_list) {
                        if (strcmp(IIO_NAME(attribute), attr) == 0) { 
                            accessors = IIO_CHN_ACCESSORS(dev);
                            if (accessors->read_attr == NULL) {
                                /* No accessor installed */
                                return -ENOSYS;
                            }
                            /* Run the accessor function */
                            return accessors->read_attr(IIO_DEV_DATA(dev), channel,
                                                        ch_out, attr, buf, len);
                        }
                    }
                    /* No matching attribute found. */
                    return -ENOENT;
                }
            }
            /* No matching channel found on device. */
            return -ENOENT;
        }
    }
    /* No matching device found. */
    return -ENOENT;
}

static ssize_t ch_write_attr(const char *device, const char *channel,
			     bool ch_out, const char *attr, const char *buf, size_t len)
{
    struct device *dev;
    struct channel *chn;
    struct attribute *attribute;
    struct device_list *dev_list;
    struct channel_list *chn_list;
    struct attribute_list *attr_list;
    struct chn_accessors *accessors;

    for_each_device(IIO_DEVICE_LIST((&context)), dev, dev_list) {
        /*
         * FIXME: atoi() does not detect errors, so we assume the input is
         * correct. Maybe this should be changed by strtol(), which is able
         * to detect errors.
         */
        if (atoi(device) == IIO_ID(dev)) {
            for_each_channel(IIO_CHANNEL_LIST(dev), chn, chn_list) {
                if (strcmp(IIO_ID(chn), channel) == 0) {
                    for_each_attribute(IIO_ATTR_LIST(chn), attribute, attr_list) {
                        if (strcmp(IIO_NAME(attribute), attr) == 0) { 
                            accessors = IIO_CHN_ACCESSORS(dev);
                            if (accessors->write_attr == NULL) {
                                /* No accessor installed */
                                return -ENOSYS;
                            }
                            /* Run the accessor function */
                            return accessors->write_attr(IIO_DEV_DATA(dev), channel,
                                                        ch_out, attr, buf, len);
                        }
                    }
                    /* No matching attribute found. */
                    return -ENOENT;
                }
            }
            /* No matching channel found on device. */
            return -ENOENT;
        }
    }
    /* No matching device found. */
    return -ENOENT;
}

ssize_t read_data(const char *device, char *buf, size_t offset,
                  size_t bytes_count)
{
    struct device *dev;
    struct device_list *dev_list;
    struct chn_accessors *accessors;

    for_each_device(IIO_DEVICE_LIST((&context)), dev, dev_list) {
        /*
         * FIXME: atoi() does not detect errors, so we assume the input is
         * correct. Maybe this should be changed by strtol(), which is able
         * to detect errors.
         */
        if (atoi(device) == IIO_ID(dev)) {
            accessors = IIO_CHN_ACCESSORS(dev);
            if (accessors->read_data == NULL) {
                /* No accessor installed */
                return -ENOSYS;
            }
            /* Run the accessor function */
            return accessors->read_data(IIO_DEV_DATA(dev), buf,
                                        offset, bytes_count);
        }
    }
    return -ENOENT;
}

ssize_t write_data(const char *device, const char *buf, size_t offset,
		           size_t bytes_count)
{
    struct device *dev;
    struct device_list *dev_list;
    struct chn_accessors *accessors;

    for_each_device(IIO_DEVICE_LIST((&context)), dev, dev_list) {
        /*
         * FIXME: atoi() does not detect errors, so we assume the input is
         * correct. Maybe this should be changed by strtol(), which is able
         * to detect errors.
         */
        if (atoi(device) == IIO_ID(dev)) {
            accessors = IIO_CHN_ACCESSORS(dev);
            if (accessors->write_data == NULL) {
                /* No accessor installed */
                return -ENOSYS;
            }
            /* Run the accessor function */
            return accessors->write_data(IIO_DEV_DATA(dev), buf,
                                         offset, bytes_count);
        }
    }
    return -ENOENT;
}

static void populate_attributes(char *xml, unsigned int *index, 
                                struct attribute_list *attrs,
                                unsigned int level)
{
    struct attribute *attr;
    struct attribute_list *_attrs;
#if 0
    char tab[6] = {0};
    unsigned int i;

    assert(level < 6);

    for(i = 0U; i < level; i++){
        strcat(tab, "\t");
    }
#endif
    for_each_attribute(attrs, attr, _attrs) {
/*        *index += level; */
        *index += strlen("<attribute name=\"");
        *index += strlen(&IIO_NAME(attr)[0]);
        *index += strlen("\" />");
        assert(*index < IIO_XML_SIZE);
/*        strcat(xml, tab); */
        strcat(xml, "<attribute name=\"");
        strcat(xml, &IIO_NAME(attr)[0]);
        strcat(xml, "\" />");
    }
}

static char *generate_xml(struct context* context)
{
    unsigned int index = strlen(dtd);
    struct device *device;
    struct device_list *_dev_list;
    struct channel *channel;
    struct channel_list *_channel_list;
    char id_str[4];
    char *xml = &context->xml[0];

    assert(IIO_XML_SIZE > index);

    strcpy(xml, dtd);

    /* Add context information */
    index += strlen("<context name=\"");
    index += strlen(&IIO_NAME(context)[0]);
    index += 2U;
    assert(index < IIO_XML_SIZE);
    strcat(xml, "<context name=\"");
    strcat(xml, &IIO_NAME(context)[0]);
    strcat(xml, "\" ");

    index += strlen("description=\"");
    index += strlen(&IIO_DESCRIPTION(context)[0]);
    index += 2U;
    assert(index < IIO_XML_SIZE);
    strcat(xml, "description=\"");
    strcat(xml, &IIO_DESCRIPTION(context)[0]);
    strcat(xml, "\">");

    /* Iterate over all the devices */
    for_each_device(IIO_DEVICE_LIST(context), device, _dev_list) {
        sprintf(id_str, "%d", IIO_ID(device));
        index += strlen("<device id=\"");
        index += strlen(id_str);
        index += strlen("\" name=\"");
        index += strlen(&IIO_NAME(device)[0]);
        index += 2U;
        assert(index < IIO_XML_SIZE);
        strcat(xml, "<device id=\"");
        strcat(xml, id_str);
        strcat(xml, "\" name=\"");
        strcat(xml, &IIO_NAME(device)[0]);
        strcat(xml, "\">");

        /* Iterate over all the channels of the device */
        for_each_channel(IIO_CHANNEL_LIST(device), channel, _channel_list) {
            index += strlen("<channel id=\"");
            index += strlen(&IIO_ID(channel)[0]);
            index += strlen("\" type=\"");
            index += strlen(&IIO_TYPE(channel)[0]);
            index += 2U;
            assert(index < IIO_XML_SIZE);
            strcat(xml, "<channel id=\"");
            strcat(xml, &IIO_ID(channel)[0]);
            strcat(xml, "\" type=\"");
            strcat(xml, &IIO_TYPE(channel)[0]);
            strcat(xml, "\">");

            populate_attributes(xml, &index, IIO_ATTR_LIST(channel), 3U);

            index += strlen("</channel>");
            assert(index < IIO_XML_SIZE);
            strcat(xml, "</channel>");
        }

        /* Populate the device attributes */
        populate_attributes(xml, &index, IIO_ATTR_LIST(device), 2U);

        index += strlen("</device>");
        assert(index < IIO_XML_SIZE);
        strcat(xml, "</device>");
    }

    index += strlen("</context>");
    assert(index < IIO_XML_SIZE);
    strcat(xml, "</context>");

    return xml;
}

static ssize_t get_xml(char **outxml)
{
    *outxml = context.xml;

	return 0;
}

struct channel *iio_new_static_channel(struct channel *channel,
                                       const char *id,
                                       const char *type)
{
    assert(channel != NULL);
    assert(id != NULL);
    assert(type != NULL);

    strncpy(&IIO_ID(channel)[0], id, CHN_ID_STR_MAX_SIZE);
    strncpy(&IIO_TYPE(channel)[0], type, TYPE_STR_MAX_SIZE);

    /* Initialise the attribute list */
    IIO_ATTR_LIST(channel) = NULL;

    return channel;
}

struct channel *iio_new_channel(const char *id,
                                const char *type)
{
    struct channel *retval = (struct channel *)malloc(sizeof(struct channel));

    if (retval == NULL) {
        return NULL;
    }

    return iio_new_static_channel(retval, id, type);
}

struct attribute *iio_new_static_attribute(struct attribute *attr,
                                           const char *name)
{
    assert(attr != NULL);
    assert(name != NULL);

    strncpy(&IIO_NAME(attr)[0], name, ATTR_NAME_MAX_SIZE);

    return attr;
}

struct attribute *iio_new_attribute(const char *name)
{
    struct attribute *retval = (struct attribute *)malloc(
                                                    sizeof(struct attribute));

    if (retval == NULL) {
        return NULL;
    }

    return iio_new_static_attribute(retval, name);
}

struct device *iio_new_static_device(struct device *device, const char *name,
                                     unsigned int id,
                                     uintptr_t data,
                                     struct attr_accessors *attr_accessors,
                                     struct chn_accessors *chn_accessors)
{
    assert(device != NULL);
    assert(name != NULL);
    assert(attr_accessors != NULL);
    assert(chn_accessors != NULL);

    strncpy(&IIO_NAME(device)[0], name, ATTR_NAME_MAX_SIZE);
    IIO_ID(device) = id;
    IIO_ATTR_ACCESSORS(device) = attr_accessors;
    IIO_CHN_ACCESSORS(device) = chn_accessors;
    IIO_DEV_DATA(device) = data;

    /* Initialise the attribute and channel lists */
    IIO_ATTR_LIST(device) = NULL;
    IIO_CHANNEL_LIST(device) = NULL;

    return device;
}

struct device *iio_new_device(const char *name,
                              unsigned int id,
                              uintptr_t data,
                              struct attr_accessors *attr_accessors,
                              struct chn_accessors *chn_accessors)
{
    struct device *retval = (struct device *)malloc(sizeof (struct device));

    if (retval == NULL) {
        return NULL;
    }

    return iio_new_static_device(retval, name, id, data,
                                 attr_accessors, chn_accessors);
}

struct context *iio_new_static_context(struct context *context,
                                       const char *name,
                                       const char *description)
{
    assert(context != NULL);
    assert(name != NULL);
    assert(description != NULL);
    assert(IIO_DEVICE_LIST(context) != NULL);

    strncpy(&IIO_NAME(context)[0], name, CONTEXT_NAME_MAX_SIZE);
    strncpy(&IIO_DESCRIPTION(context)[0], description, CONTEXT_DESC_MAX_SIZE);

    return context;
}

struct context *iio_new_context(const char *name,
                                const char *description)
{
    struct context *retval = (struct context *)malloc(sizeof(struct context));

    if (retval == NULL) {
        return NULL;
    }

    return iio_new_static_context(retval, name, description);
}

int iio_register_attribute(struct attribute *attr,
                           struct attribute_list **list)
{
    struct attribute *attribute;
    struct attribute_list *_attr_list;
    int retval = 0;

    assert(attr != NULL);

    if (*list == NULL) {
        *list = (struct attribute_list *)calloc(1, sizeof(struct attribute_list));
        if (*list == NULL) {
            return -ENOMEM;
        }
    }

    /*
     * Special case: If the pointer to attr is NULL, this is the first element
     * on the list.
     */
    if (IIO_ATTR((*list)) == NULL) {
        IIO_ATTR((*list)) = attr;
        return 1;
    }

    /* 
     * Iterate over all the attributes untile reaching the end of the list
     * or an attribute with a similar name.
     */ 

    for_each_attribute((*list), attribute, _attr_list) {
        if (strcmp(&IIO_NAME(attr)[0], &IIO_NAME(attribute)[0]) == 0) {
            return -EINVAL;
        }
        retval++;
    }

    /* _attr_list is now pointing to the last element of the list */
    IIO_NEXT_ITEM(_attr_list) =
                (struct attribute_list *)calloc(1,
                                        sizeof(struct attribute_list));
    if (IIO_NEXT_ITEM(_attr_list) == NULL) {
        return -EINVAL;
    }
    _attr_list = IIO_NEXT_ITEM(_attr_list);
    IIO_ATTR(_attr_list) = attr;
    IIO_NEXT_ITEM(_attr_list) = NULL;

    return retval + 1;
}

int iio_register_channel(struct channel *channel,
                         struct channel_list **list)
{
    struct channel *chn;
    struct channel_list *_list;
    int retval = 0;

    assert(channel != NULL);

    if (*list == NULL) {
        *list = (struct channel_list *)calloc(1, sizeof(struct channel_list));
        if (*list == NULL) {
            return -ENOMEM;
        }
    }

    /*
     * Special case: If the pointer to the channel is NULL, this is the first
     * element on the list.
     */
    if (IIO_CHANNEL((*list)) == NULL) {
        IIO_CHANNEL((*list)) = channel;
        return 1;
    }

    /* 
     * Iterate over all the attributes untile reaching the end of the list
     * or a channel with a similar name.
     */ 
    for_each_channel((*list), chn, _list) {
        if (strcmp(&IIO_TYPE(chn)[0], &IIO_TYPE(channel)[0]) == 0 &&
            strcmp(&IIO_ID(chn)[0], &IIO_ID(channel)[0]) == 0) {
            return -EINVAL;
        }
        retval++;
    }

    /* _attr_list is now pointing to the last element of the list */
    IIO_NEXT_ITEM(_list) =
                (struct channel_list *)calloc(1,
                                        sizeof(struct channel_list));
    if (IIO_NEXT_ITEM(_list) == NULL) {
        return -EINVAL;
    }
    _list = IIO_NEXT_ITEM(_list);
    IIO_CHANNEL(_list) = channel;
    IIO_NEXT_ITEM(_list) = NULL;

    return retval + 1;
}

int iio_register_device(struct device *device, struct device_list **list)
{
    struct device *dev;
    struct device_list *_list;
    int retval = 0;

    assert(device != NULL);

    if (*list == NULL) {
        *list = (struct device_list *)calloc(1, sizeof(struct device_list));
        if (*list == NULL) {
            return -ENOMEM;
        }
    }

    /*
     * Special case: If the pointer to the channel is NULL, this is the first
     * element on the list.
     */
    if (IIO_DEVICE((*list)) == NULL) {
        IIO_DEVICE((*list)) = device;
        return 1;
    }

    /* 
     * Iterate over all the attributes until reaching the end of the list
     * or a channel with a similar name.
     */ 
    for_each_device((*list), dev, _list) {
        if (strcmp(&IIO_NAME(dev)[0], &IIO_NAME(device)[0]) == 0 ||
            IIO_ID(dev) == IIO_ID(dev)) {
            return -EINVAL;
        }
        retval++;
    }

    /* _list is now pointing to the last element of the list */
    IIO_NEXT_ITEM(_list) =
                (struct device_list *)calloc(1,
                                        sizeof(struct device_list));
    if (IIO_NEXT_ITEM(_list) == NULL) {
        return -EINVAL;
    }
    _list = IIO_NEXT_ITEM(_list);
    IIO_DEVICE(_list) = device;
    IIO_NEXT_ITEM(_list) = NULL;

    return retval + 1;
}

struct context *iio_init(const char *ctx_name, const char *ctx_desc,
                         struct device_list *devices,
                         iio_write_cb_t write_cb, iio_read_cb_t read_cb)
{
    assert(ctx_name != NULL);
    assert(ctx_desc != NULL);
    assert(devices != NULL);
    assert(write_cb != NULL);
    assert(read_cb != NULL);

    strncpy(&IIO_NAME((&context))[0], ctx_name, CONTEXT_NAME_MAX_SIZE);
    strncpy(&IIO_DESCRIPTION((&context))[0], ctx_desc, CONTEXT_DESC_MAX_SIZE);
    IIO_DEVICE_LIST((&context)) = devices;

    context.ops.read_attr = read_attr;
    context.ops.write_attr = write_attr;
    context.ops.ch_read_attr = ch_read_attr;
    context.ops.ch_write_attr = ch_write_attr;
    context.ops.read_data = read_data;
    context.ops.write_data = write_data;
    context.ops.get_xml = get_xml;

    context.ops.write = write_cb;
    context.ops.read = read_cb;

    context.iiod = tinyiiod_create(&context.ops);
    generate_xml(&context);

    return &context;
}

int iio_read_command(struct context *context)
{
    assert(context != NULL);
    assert(context->ops.read != NULL);
    assert(context->iiod != NULL);

    return tinyiiod_read_command(context->iiod);
}