CFLAGS := $(shell pkg-config --cflags glib-2.0)
LDFLAGS := $(shell pkg-config --libs glib-2.0)
CC := gcc

CFLAGS += -DINCLUDE_TESTS

.SUFFIX:

%.a:
	-rm -f $@
	ar crv $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f $(OUTPUTS)
