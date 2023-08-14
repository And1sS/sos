BUILD_FOLDER = build_output
H_FILES = types.h vga_print.h
OBJ_FILES = $(BUILD_FOLDER)/kernel.o $(BUILD_FOLDER)/vga_print.o
CROSS_COMPILE = x86_64-elf-
ASM = nasm
CC = gcc
CC_FLAGS = -c -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
		   -Wall -Wextra -Werror
LINKER = ld

build: $(BUILD_FOLDER) $(BUILD_FOLDER)/sos_kernel.elf

$(BUILD_FOLDER):
	mkdir -p $(BUILD_FOLDER)

$(BUILD_FOLDER)/sos_kernel.elf: $(BUILD_FOLDER)/bootstrap.elf $(OBJ_FILES)
	$(CROSS_COMPILE)$(LINKER) -melf_i386 -Tlinker.ld \
    		$(BUILD_FOLDER)/bootstrap.elf $(OBJ_FILES) -o $(BUILD_FOLDER)/sos-kernel.elf

$(BUILD_FOLDER)/%.o: %.c $(H_FILES)
	$(CROSS_COMPILE)$(CC) $(CC_FLAGS) -o $@ $<

$(BUILD_FOLDER)/bootstrap.elf: bootstrap.asm
	$(ASM) -f elf32 -o $@ $<

iso: build grub.cfg
	mkdir -p $(BUILD_FOLDER)/iso/boot/grub
	cp grub.cfg $(BUILD_FOLDER)/iso/boot/grub/grub.cfg
	cp $(BUILD_FOLDER)/sos-kernel.elf $(BUILD_FOLDER)/iso/boot/sos-kernel.elf
	grub-mkrescue -o $(BUILD_FOLDER)/grub.iso $(BUILD_FOLDER)/iso

all: iso

run: iso
	qemu-system-x86_64 -cdrom $(BUILD_FOLDER)/grub.iso

clean:
	rm -rf build
