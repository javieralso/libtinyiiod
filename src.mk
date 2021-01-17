SRCS := $(ROOT)/parser.c			\
	$(ROOT)/tinyiiod.c

UTESTS := example

ifeq ($(WRAPPER),1)
SRCS += $(ROOT)/iio.c
UTESTS += example2
endif
