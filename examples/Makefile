
OUTPUTS := dns.o \
	   dns

TOPLEVEL := ../

include ../common.mk


all: dns

dns: dns.o
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

dns.o: ../src/hammer.h
