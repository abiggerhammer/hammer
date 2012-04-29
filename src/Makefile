-include ../common.mk

all: libhammer.a test_suite

test_suite: test_suite.o libhammer.a
	$(CC) $(LDFLAGS) -o $@ $^

libhammer.a: bitreader.o

bitreader.o: test_suite.h
