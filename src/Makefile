-include ../common.mk

OUTPUTS := bitreader.o \
	   libhammer.a \
	   test_suite

all: libhammer.a test_suite

test_suite: test_suite.o libhammer.a
	$(CC) -o $@ $^ $(LDFLAGS)

libhammer.a: bitreader.o

bitreader.o: test_suite.h
