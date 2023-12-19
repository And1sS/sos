BUILD_FOLDER = build_output
SUBDIRS = $(filter-out $(BUILD_FOLDER)/., $(wildcard */.))

all: $(SUBDIRS)

clean:
	rm -rf $(BUILD_FOLDER)


$(SUBDIRS): FORCE
	$(MAKE) -C $@
	python3 ../clone_elfs.py $@

FORCE:

.PHONY: all $(SUBDIRS)
