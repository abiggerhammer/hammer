
OUTPUTS := dns.o \
	   dns

TOPLEVEL := ../

include ../common.mk


all: dns

dns: LDFLAGS:=-L../src -lhammer $(LDFLAGS)
dns: dns.o rr.o dns_common.o
	$(call hush, "Linking $@") $(CC) -o $@ $^ $(LDFLAGS)

dns.o: ../src/hammer.h dns_common.h

rr.o: ../src/hammer.h rr.h dns_common.h

dns_common.o: ../src/hammer.h dns_common.h