BUILD_FOLDER = build_output
ASM = nasm
BOOTSTRAP_FILE = bootstrap.asm
KERNEL_FILE = kernel.c
CROSS_COMPILE = x86_64-elf-
CC = gcc
CC_FLAGS = -c -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -nostartfiles -nodefaultlibs \
		   -Wall -Wextra -Werror
LINKER = ld

build: $(BOOTSTRAP_FILE) $(KERNEL_FILE)
	mkdir -p $(BUILD_FOLDER)
	$(ASM) -f elf32 $(BOOTSTRAP_FILE) -o $(BUILD_FOLDER)/bootstrap.elf
	
	$(CROSS_COMPILE)$(CC) $(CC_FLAGS) kernel.c -o $(BUILD_FOLDER)/kernel.elf
	$(CROSS_COMPILE)$(LINKER) -melf_i386 -Tlinker.ld \
		$(BUILD_FOLDER)/bootstrap.elf $(BUILD_FOLDER)/kernel.elf -o $(BUILD_FOLDER)/sos-kernel.elf

iso: build grub.cfg
	mkdir -p $(BUILD_FOLDER)/iso/boot/grub
	cp grub.cfg $(BUILD_FOLDER)/iso/boot/grub/grub.cfg
	cp $(BUILD_FOLDER)/sos-kernel.elf $(BUILD_FOLDER)/iso/boot/sos-kernel.elf
	grub-mkrescue -o $(BUILD_FOLDER)/grub.iso $(BUILD_FOLDER)/iso

run: iso
	qemu-system-x86_64 -cdrom $(BUILD_FOLDER)/grub.iso

clean:
	rm -rf build
