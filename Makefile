# Most of this is so that you can do
# $ make all
# and kick off a recursive make
# Also, "make src/all" turns into "make -C src all"

SUBDIRS = src examples jni

include config.mk
TOPLEVEL=.
include common.mk

CONFIG_VARS= INCLUDE_TESTS

.DEFAULT_GOAL := all

nojni: all
nojni: SUBDIRS:=$(filter-out jni,$(SUBDIRS))

all clean:
	+for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done

test: src/test_suite
	$<

examples/all: src/all
examples/compile: src/compile

define SUBDIR_TEMPLATE
$(1)/%: force
	$(MAKE) -C $(1) $$*
endef

force:

$(foreach dir,$(SUBDIRS),$(eval $(call SUBDIR_TEMPLATE,$(dir))))

#.DEFAULT:
#	$(if $(findstring ./,$(dir $@)),$(error No rule to make target `$@'),$(MAKE) -C $(dir $@) $(notdir $@))

TAGS: force
	etags $(shell find * -name "*.c" -o -name "*.h")

config:
	@printf "%30s %s\n" $(foreach var,$(CONFIG_VARS),$(var) $($(var)) )
