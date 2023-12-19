BUILD_FOLDER = build_output

H_FILES := $(shell find ./ -name '*.h')
С_FILES := $(shell find ./ -name '*.c')
OBJ_FILES := $(patsubst ./%.c, $(BUILD_FOLDER)/%.o, $(С_FILES))

RESULT_ELF = $(BUILD_FOLDER)/test.elf

CROSS_COMPILE =
CC = gcc
CC_FLAGS = -c -g -O3 -m64 -mcmodel=large -mgeneral-regs-only -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-red-zone -nostartfiles -nodefaultlibs \
		   -Wall -Wextra -Werror

LINKER = ld

all: $(RESULT_ELF)

clean:
	rm -rf $(BUILD_FOLDER)

$(RESULT_ELF): $(OBJ_FILES)
	$(CROSS_COMPILE)$(LINKER) -melf_x86_64 $(OBJ_FILES) -o $(RESULT_ELF)

$(BUILD_FOLDER)/%.o: ./%.c $(H_FILES)
	mkdir -p $(@D)
	$(CROSS_COMPILE)$(CC) $(CC_FLAGS) -o $@ $<

.PHONY: all
