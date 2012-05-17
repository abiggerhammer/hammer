CFLAGS := $(shell pkg-config --cflags glib-2.0) -std=gnu99 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-attributes
LDFLAGS := $(shell pkg-config --libs glib-2.0)
CC := gcc
# Set V=1 for verbose mode...
V := 0
CFLAGS += -DINCLUDE_TESTS $(EXTRA_CFLAGS)
HUSH = $(TOPLEVEL)/lib/hush

# Check to make sure variables are properly set
ifeq ($(TOPLEVEL),)
$(error $$TOPLEVEL is unset)
endif

ifsilent = $(if $(findstring 0, $(V)),$(1),)
hush = $(call ifsilent,$(HUSH) $(1))
#.SUFFIXES:

ifeq ($(V),0)
.SILENT:
endif

.DEFAULT_GOAL:=all

%.a: $(call ifsilent,| $(HUSH))
	-rm -f $@
	$(call hush,"Archiving $@") ar cr $@ $^


%.o: %.c $(call ifsilent,| $(HUSH))
	$(call hush, "Compiling $<") $(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f $(OUTPUTS)

$(TOPLEVEL)/lib/hush: $(TOPLEVEL)/lib/hush.c
	make -C $(TOPLEVEL)/lib hush
