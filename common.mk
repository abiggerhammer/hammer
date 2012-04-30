CFLAGS := $(shell pkg-config --cflags glib-2.0) -std=gnu99 -Wall -Wextra -Werror
LDFLAGS := $(shell pkg-config --libs glib-2.0)
CC := gcc
# Set V=1 for verbose mode...
V := 0
CFLAGS += -DINCLUDE_TESTS
HUSH = $(TOPLEVEL)/lib/hush

# Check to make sure variables are properly set
ifeq ($(TOPLEVEL),)
$(error $$TOPLEVEL is unset)
endif

#.SUFFIXES:

ifeq ($(V),0)
.SILENT:
endif

.DEFAULT_GOAL:=all

%.a: | $(HUSH)
	-rm -f $@
	$(if $(findstr 0,$(V)),$(HUSH) "Archiving $@",) ar cr $@ $^


ifeq ($(V),0)
# silent mode
%.o: %.c | $(HUSH)
	$(HUSH) "Compiling $<" $(CC) $(CFLAGS) -c -o $@ $<
else
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
endif
clean:
	-rm -f $(OUTPUTS)

$(TOPLEVEL)/lib/hush: $(TOPLEVEL)/lib/hush.c
	make -C $(TOPLEVEL)/lib hush