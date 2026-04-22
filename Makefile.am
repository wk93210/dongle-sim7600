# chan_dongle - SIM7600/SIMCOM Channel Driver for Asterisk

ACLOCAL_AMFLAGS = -I m4

# Subdirectories
SUBDIRS = . tests

# The module
module_LTLIBRARIES = chan_dongle.la
moduledir = $(ASTERISK_MODULES)

# Source files
chan_dongle_la_SOURCES = \
	app.c \
	at_command.c \
	at_parse.c \
	at_queue.c \
	at_read.c \
	at_response.c \
	chan_dongle.c \
	channel.c \
	char_conv.c \
	cli.c \
	helpers.c \
	manager.c \
	memmem.c \
	ringbuffer.c \
	cpvt.c \
	dc_config.c \
	pdu.c \
	mixbuffer.c \
	pdiscovery.c \
	error.c \
	smsdb.c

# Header files (for distribution)
noinst_HEADERS = \
	app.h \
	ast_compat.h \
	ast_config.h \
	at_command.h \
	at_parse.h \
	at_queue.h \
	at_read.h \
	at_response.h \
	chan_dongle.h \
	channel.h \
	char_conv.h \
	cli.h \
	config.h \
	cpvt.h \
	dc_config.h \
	error.h \
	export.h \
	gsm7_luts.h \
	helpers.h \
	manager.h \
	memmem.h \
	mixbuffer.h \
	mutils.h \
	pdiscovery.h \
	pdu.h \
	preinclude.h \
	ringbuffer.h \
	smsdb.h

# Compiler flags
chan_dongle_la_CFLAGS = \
	-fPIC \
	-O2 \
	-g \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-std=gnu11 \
	-D_GNU_SOURCE \
	-DHAVE_CONFIG_H \
	-DAST_MODULE_SELF_SYM=__internal_chan_dongle_self \
	-DAST_MODULE=\"chan_dongle\" \
	-I$(top_srcdir) \
	-include $(top_srcdir)/preinclude.h \
	$(SQLITE3_CFLAGS) \
	$(ICONV_CFLAGS) \
	-idirafter $(ASTERISK_INCLUDE_DIR)/asterisk

# Linker flags
chan_dongle_la_LDFLAGS = \
	-module \
	-avoid-version \
	-shared \
	-fPIC \
	$(SQLITE3_LIBS) \
	$(ICONV_LIBS) \
	-lpthread

# Extra files to distribute
EXTRA_DIST = \
	etc/dongle.conf \
	etc/extensions.conf \
	README.md \
	LICENSE \
	.pre-commit-config.yaml

# Install configuration example
install-data-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/asterisk
	@if test ! -f $(DESTDIR)$(sysconfdir)/asterisk/dongle.conf; then \
		echo "Installing sample configuration..."; \
		$(INSTALL_DATA) $(srcdir)/etc/dongle.conf $(DESTDIR)$(sysconfdir)/asterisk/dongle.conf.sample; \
	else \
		echo "Configuration file exists, installing sample as dongle.conf.sample..."; \
		$(INSTALL_DATA) $(srcdir)/etc/dongle.conf $(DESTDIR)$(sysconfdir)/asterisk/dongle.conf.sample; \
	fi

# Uninstall
uninstall-local:
	@$(NORMAL_UNINSTALL)
	rm -f $(DESTDIR)$(ASTERISK_MODULES)/chan_dongle.so

# Clean
CLEANFILES = *~

# Maintainer clean
MAINTAINERCLEANFILES = \
	Makefile.in \
	aclocal.m4 \
	compile \
	config.guess \
	config.h.in \
	config.sub \
	configure \
	depcomp \
	install-sh \
	ltmain.sh \
	missing
