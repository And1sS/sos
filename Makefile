BUILD_FOLDER = build_output

H_FILES := $(shell find ./source -name '*.h')
SOURCE_FILES := $(shell find ./source -name '*.c')
OBJ_FILES := $(patsubst ./source/%.c, $(BUILD_FOLDER)/%.o, $(SOURCE_FILES))

BOOTSTRAP_ELF = $(BUILD_FOLDER)/bootstrap.elf
KERNEL_ELF = $(BUILD_FOLDER)/sos_kernel.elf

ISO_GRUB_CFG = $(BUILD_FOLDER)/iso/boot/grub/grub.cfg
ISO_KERNEL_FILE = $(BUILD_FOLDER)/iso/boot/sos-kernel.elf

ISO_FILE = $(BUILD_FOLDER)/sos.iso

CROSS_COMPILE = x86_64-elf-
ASM = nasm
CC = gcc
CC_FLAGS = -c -g -O3 -m64 -mcmodel=large -mgeneral-regs-only -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mno-red-zone -nostartfiles -nodefaultlibs \
		   -Wall -Wextra -Werror
QEMU_FLAGS = -D ./log.txt -d int,cpu_reset -no-reboot
LINKER = ld

all: iso

run: iso
	qemu-system-x86_64 -cdrom $(ISO_FILE) $(QEMU_FLAGS)

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

$(KERNEL_ELF): $(BOOTSTRAP_ELF) $(OBJ_FILES) linker.ld
	$(CROSS_COMPILE)$(LINKER) -melf_x86_64 -z max-page-size=0x1000 -Tlinker.ld $(BOOTSTRAP_ELF) $(OBJ_FILES) -o $(KERNEL_ELF)

$(BOOTSTRAP_ELF): source/x86_64/bootstrap.asm
	mkdir -p $(@D)
	$(ASM) -f elf64 -o $@ $<

$(BUILD_FOLDER)/%.o: source/%.c $(H_FILES)
	mkdir -p $(@D)
	$(CROSS_COMPILE)$(CC) $(CC_FLAGS) -o $@ $<
