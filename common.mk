CFLAGS := $(shell pkg-config --cflags glib-2.0) -std=gnu99
LDFLAGS := $(shell pkg-config --libs glib-2.0)
CC := gcc

CFLAGS += -DINCLUDE_TESTS

.SUFFIX:

.DEFAULT_GOAL:=all

%.a:
	-rm -f $@
	ar crv $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f $(OUTPUTS)
