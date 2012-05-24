
OUTPUTS := dns.o \
	   dns

TOPLEVEL := ../

include ../common.mk


all: dns

dns: LDFLAGS:=-L../src -lhammer $(LDFLAGS)
dns: dns.o
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

dns.o: ../src/hammer.h
