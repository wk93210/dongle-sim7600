# chan_dongle - SIM7600/SIMCOM Channel Driver for Asterisk
# Copyright (C) 2025

CC = gcc

# Use idirafter to ensure system headers come first
CFLAGS = -fPIC -O2 -g -Wall -Wextra -Wno-unused-parameter -std=gnu11 \
    -DASTERISK_VERSION_NUM=200190 \
    -DAST_MODULE_SELF_SYM=__internal_chan_dongle_self \
    -DAST_MODULE=\"chan_dongle\" \
    -D_GNU_SOURCE \
    -DHAVE_CONFIG_H \
    -I. \
    -include $(PWD)/preinclude.h \
    -idirafter /usr/include/asterisk

LDFLAGS = -shared -fPIC -lsqlite3 -lpthread

SRCS = app.c at_command.c at_parse.c at_queue.c at_read.c at_response.c \
    chan_dongle.c channel.c char_conv.c cli.c helpers.c manager.c \
    memmem.c ringbuffer.c cpvt.c dc_config.c pdu.c mixbuffer.c pdiscovery.c error.c smsdb.c

OBJS = $(SRCS:.c=.o)

TARGET = chan_dongle.so

.PHONY: all clean install test

all: $(TARGET)

test:
	$(MAKE) -C tests test

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	install -D -m 644 $(TARGET) /usr/lib/asterisk/modules/$(TARGET)
	@echo "Module installed. Restart Asterisk to load."
