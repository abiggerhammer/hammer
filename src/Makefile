
OUTPUTS := bitreader.o \
	   hammer.o \
	   libhammer.a \
	   test_suite

TOPLEVEL := ../

include ../common.mk


all: libhammer.a test_suite

test_suite: test_suite.o libhammer.a
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

libhammer.a: bitreader.o hammer.o pprint.o

bitreader.o: test_suite.h
hammer.o: hammer.h

test: test_suite
	./test_suite -v
