H_FILES := $(shell find ./ -name '*.h')
С_FILES := $(shell find ./ -name '*.c')
OBJ_FILES := $(patsubst ./%.c, $(BUILD_FOLDER)/%.o, $(С_FILES))

ASM_FILES := $(shell find ./ -name '*.asm')
ELF_FILES := $(patsubst ./%.asm, $(BUILD_FOLDER)/%.elf, $(ASM_FILES))

KERNEL_ELF = $(BUILD_FOLDER)/sos-kernel.elf

CROSS_COMPILE =
ASM = nasm
CC = gcc
CC_FLAGS = -c -g -O3 -m64 -mcmodel=large -mgeneral-regs-only -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-red-zone -nostartfiles -nodefaultlibs \
		   -Wall -Wextra -Werror
LINKER = ld

all: $(KERNEL_ELF)

clean:
	rm -rf $(BUILD_FOLDER)

$(KERNEL_ELF): $(ELF_FILES) $(OBJ_FILES) linker.ld
	$(CROSS_COMPILE)$(LINKER) -melf_x86_64 -z max-page-size=0x1000 -Tlinker.ld $(ELF_FILES) $(OBJ_FILES) -o $(KERNEL_ELF)

$(BUILD_FOLDER)/%.elf: ./%.asm
	mkdir -p $(@D)
	$(ASM) -f elf64 -o $@ $<

$(BUILD_FOLDER)/%.o: ./%.c $(H_FILES)
	mkdir -p $(@D)
	$(CROSS_COMPILE)$(CC) $(CC_FLAGS) -o $@ $<
