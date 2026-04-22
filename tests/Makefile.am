# Tests for chan_dongle

noinst_PROGRAMS = test_at_parse

# Standalone test - includes its own implementations
test_at_parse_SOURCES = test_at_parse.c

test_at_parse_CFLAGS = \
	-O2 -g -Wall -std=gnu11 \
	-D_GNU_SOURCE \
	-I$(top_srcdir)

# Automake test harness
TESTS = test_at_parse
TEST_EXTENSIONS = .c
AM_TESTS_FD_REDIRECT = 2>&1

# Additional check target for manual runs
check-local: test_at_parse
	$(AM_V_at)./test_at_parse

.PHONY: test
test: check
