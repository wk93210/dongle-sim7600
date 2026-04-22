# Test Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g -I..
LDFLAGS =

TEST_SRCS = test_at_parse.c
TEST_OBJS = $(TEST_SRCS:.c=.o)
TEST_BINS = test_at_parse

.PHONY: all clean test

all: $(TEST_BINS)

test_at_parse: test_at_parse.o
	$(CC) $(LDFLAGS) -o $@ $^

test_at_parse.o: test_at_parse.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TEST_BINS)
	@echo "Running tests..."
	@for test in $(TEST_BINS); do \
		echo "\n=== Running $$test ==="; \
		./$$test || exit 1; \
	done
	@echo "\n=== All tests passed ==="

clean:
	rm -f $(TEST_OBJS) $(TEST_BINS)
