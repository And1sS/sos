RESULT_BINARY = $(BUILD_FOLDER)test.bin

H_FILES := $(shell find ./ -name '*.h')
С_FILES := $(shell find ./ -name '*.c')
OBJ_FILES := $(patsubst ./%.c, $(BUILD_FOLDER)%.o, $(С_FILES))

CROSS_COMPILE =
CC = gcc
CC_FLAGS = -r -c -g -O3 -w -nostdlib -nostdinc -nostartfiles -nodefaultlibs -mmanual-endbr -fcf-protection=branch

LINKER = ld

all: $(RESULT_BINARY)

clean:
	rm -rf $(BUILD_FOLDER)

$(RESULT_BINARY): $(OBJ_FILES) linker.ld
	$(CROSS_COMPILE)$(LINKER) --oformat binary -z max-page-size=0x1000 -Tlinker.ld $(OBJ_FILES) -o $(RESULT_BINARY)


$(BUILD_FOLDER)%.o: ./%.c $(H_FILES)
	mkdir -p $(@D)
	$(CROSS_COMPILE)$(CC) $(CC_FLAGS) -o $@ $<


.PHONY: all
