BUILD_FOLDER = build_output

H_FILES := $(shell find . -name '*.h')
SOURCE_FILES := $(shell find . -name '*.c')
OBJ_FILES := $(patsubst %.c, $(BUILD_FOLDER)/%.o, $(SOURCE_FILES))

BOOTSTRAP_ELF = $(BUILD_FOLDER)/bootstrap.elf
KERNEL_ELF = $(BUILD_FOLDER)/sos_kernel.elf

ISO_GRUB_CFG = $(BUILD_FOLDER)/iso/boot/grub/grub.cfg
ISO_KERNEL_FILE = $(BUILD_FOLDER)/iso/boot/sos-kernel.elf

ISO_FILE = $(BUILD_FOLDER)/sos.iso

CROSS_COMPILE = x86_64-elf-
ASM = nasm
CC = gcc
CC_FLAGS = -c -g -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
		   -Wall -Wextra -Werror
LINKER = ld

all: iso

run: iso
	qemu-system-x86_64 -cdrom $(ISO_FILE)

iso: $(ISO_FILE)

clean:
	rm -rf $(BUILD_FOLDER)

$(ISO_FILE): $(ISO_KERNEL_FILE) $(ISO_GRUB_CFG)
	grub-mkrescue -o $(ISO_FILE) $(BUILD_FOLDER)/iso

$(ISO_KERNEL_FILE): $(KERNEL_ELF)
	mkdir -p $(@D)
	cp $< $@

$(ISO_GRUB_CFG): grub.cfg
	mkdir -p $(@D)
	cp $< $@

$(KERNEL_ELF): $(BOOTSTRAP_ELF) $(OBJ_FILES)
	$(CROSS_COMPILE)$(LINKER) -melf_i386 -Tlinker.ld $(BOOTSTRAP_ELF) $(OBJ_FILES) -o $(KERNEL_ELF)

$(BOOTSTRAP_ELF): bootstrap.asm
	mkdir -p $(@D)
	$(ASM) -f elf32 -o $@ $<

$(BUILD_FOLDER)/%.o: %.c $(H_FILES)
	mkdir -p $(@D)
	$(CROSS_COMPILE)$(CC) $(CC_FLAGS) -o $@ $<
