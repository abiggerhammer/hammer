# Most of this is so that you can do
# $ make all
# and kick off a recursive make
# Also, "make src/all" turns into "make -C src all"

SUBDIRS = src \
	  lib

.DEFAULT_GOAL := all

%:
	+for dir in $(SUBDIRS); do $(MAKE) -C $${dir} $@; done

test: src/test_suite
	$<

define SUBDIR_TEMPLATE
$(1)/%:
	$$(MAKE) -C $(1) $$*
endef

$(foreach dir,$(SUBDIRS),$(eval $(call SUBDIR_TEMPLATE,$(dir))))

#.DEFAULT:
#	$(if $(findstring ./,$(dir $@)),$(error No rule to make target `$@'),$(MAKE) -C $(dir $@) $(notdir $@))
