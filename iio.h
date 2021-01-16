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

#ifndef __IIO_H__
#define __IIO_H__

#include <unistd.h>
#include "tinyiiod.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IIO_XML_SIZE
#define IIO_XML_SIZE                (1024 * 3)
#endif

#define ATTR_NAME_MAX_SIZE          32
#define DEV_NAME_STR_MAX_SIZE       32
#define CHN_ID_STR_MAX_SIZE         32
#define CONTEXT_NAME_MAX_SIZE       32
#define CONTEXT_DESC_MAX_SIZE       32
#define TYPE_STR_MAX_SIZE           16

/** Simple attribute */
struct attribute {
    char name[ATTR_NAME_MAX_SIZE];
};

/** List of attributes. */
struct attribute_list {
    struct attribute *attribute;
    struct attribute_list *next;
};

/** Simple channel. */
struct channel {
    char id[CHN_ID_STR_MAX_SIZE];
    char type[TYPE_STR_MAX_SIZE];
    struct attribute_list *attrs;
};

/** List of channels. */
struct channel_list {
    struct channel *channel;
    struct channel_list *next;
};

/** Attribute accessors */
struct attr_accessors {
    ssize_t (*read_attr)(uintptr_t data, const char *attr,
                         char *buf, size_t len, enum iio_attr_type type);
    ssize_t (*write_attr)(uintptr_t data, const char *attr,
                          const char *buf, size_t len, enum iio_attr_type type);
};

/** Channel accessors */
struct chn_accessors {
    ssize_t (*read_attr)(uintptr_t data, const char *channel,
             bool ch_out, const char *attr, char *buf,
             ssize_t len);
    ssize_t (*write_attr)(uintptr_t data, const char *channel,
             bool ch_out, const char *attr, const char *buf,
             ssize_t len);
    ssize_t (*read_data)(uintptr_t data, char *buf, size_t offset,
             size_t bytes_count);
    ssize_t (*write_data)(uintptr_t data, const char* buf, size_t offset,
             size_t bytes_count);
};

/** Device */
struct device {
    char name[DEV_NAME_STR_MAX_SIZE];
    unsigned int id;
    uintptr_t data;
    struct channel_list *channels;
    struct attribute_list *attrs;
    struct attr_accessors *attr_accessors;
    struct chn_accessors *chn_accessors;
};

/** List of devices */
struct device_list {
    struct device *device;
    struct device_list *next;
}; 

/** Context */
struct context {
    char name[CONTEXT_NAME_MAX_SIZE];
    char description[CONTEXT_DESC_MAX_SIZE];
    struct device_list *devices;
    char xml[IIO_XML_SIZE];
    struct tinyiiod *iiod;
    struct tinyiiod_ops ops;
};

/** Accessors to the struct elements */
#define IIO_NAME(x)             (x->name)
#define IIO_ID(x)               (x->id)
#define IIO_TYPE(x)             (x->type)
#define IIO_DESCRIPTION(x)      (x->description)
#define IIO_ATTR(x)             (x->attribute)
#define IIO_ATTR_LIST(x)        (x->attrs)
#define IIO_CHANNEL(x)          (x->channel)
#define IIO_CHANNEL_LIST(x)     (x->channels)
#define IIO_DEVICE(x)           (x->device)
#define IIO_DEVICE_LIST(x)      (x->devices)
#define IIO_NEXT_ITEM(x)        (x->next)
#define IIO_ATTR_ACCESSORS(x)   (x->attr_accessors)
#define IIO_CHN_ACCESSORS(x)    (x->chn_accessors)
#define IIO_DEV_DATA(x)         (x->data)

typedef ssize_t (*iio_write_cb_t)(const char *buf, size_t len);
typedef ssize_t (*iio_read_cb_t)(char *buf, size_t len);

struct channel *iio_new_channel(const char *id,
                                const char *type);

struct attribute *iio_new_attribute(const char *name);

struct device *iio_new_device(const char *name,
                              unsigned int id,
                              uintptr_t data,
                              struct attr_accessors *attr_accessors,
                              struct chn_accessors *chn_accessors);

struct context *iio_new_context(const char *name,
                                const char *description);

struct channel *iio_new_static_channel(struct channel *channel,
                                       const char *id,
                                       const char *type);
struct attribute *iio_new_static_attribute(struct attribute *attr,
                                           const char *name);
struct device *iio_new_static_device(struct device *device, const char *name,
                                     unsigned int id,
                                     uintptr_t data,
                                     struct attr_accessors *attr_accessors,
                                     struct chn_accessors *chn_accessors);
struct context *iio_new_static_context(struct context *context,
                                       const char *name,
                                       const char *description);
int iio_register_attribute(struct attribute *attr,
                           struct attribute_list **list);
int iio_register_channel(struct channel *channel,
                         struct channel_list **list);
int iio_register_device(struct device *device,
                        struct device_list **list);
struct context *iio_init(const char *ctx_name, const char *ctx_desc,
                         struct device_list *devices,
                         iio_write_cb_t write_cb, iio_read_cb_t read_cb);
int iio_read_command(struct context* context);

#define for_each_channel(_list, _channel, _listptr)                 \
    for (_listptr = _list, _channel = _list->channel;               \
         _channel != NULL;                \
         _channel = _listptr->next != NULL ? _listptr->next->channel :  NULL, \
         _listptr = _listptr->next != NULL ? _listptr->next : _listptr)

#define for_each_attribute(_list, _attribute, _listptr)             \
    for (_listptr = _list, _attribute = _list->attribute;           \
         _attribute != NULL;                \
         _attribute = _listptr->next != NULL ? _listptr->next->attribute :  NULL, \
         _listptr = _listptr->next != NULL ? _listptr->next : _listptr)

#define for_each_device(_list, _device, _listptr)                   \
    for (_listptr = _list, _device = _list->device;                 \
         _device != NULL;                \
         _device = _listptr->next != NULL ? _listptr->next->device :  NULL, \
         _listptr = _listptr->next != NULL ? _listptr->next : _listptr)

#ifdef __cplusplus
}           /* extern "C" */
#endif
#endif /* __IIO_H__ */
