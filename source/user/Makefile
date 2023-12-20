SUBDIRS = $(wildcard */.)

all: $(SUBDIRS)

clean:
	rm -rf $(BUILD_FOLDER)

$(SUBDIRS): FORCE
	$(MAKE) BUILD_FOLDER=$(BUILD_FOLDER)$(shell echo $@ | rev | cut -d'/' -f2- | rev) -C $@

FORCE:

.PHONY: all $(SUBDIRS)
