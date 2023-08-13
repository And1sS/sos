BUILD_FOLDER = build
ASM = nasm
BOOTSTRAP_FILE = bootstrap.asm
CROSS_COMPILE = x86_64-elf-

build: $(BOOTSTRAP_FILE)
	mkdir -p $(BUILD_FOLDER)
	$(ASM) -f elf32 $(BOOTSTRAP_FILE) -o $(BUILD_FOLDER)/bootstrap.elf
	
	$(CROSS_COMPILE)ld -melf_i386 -Tlinker.ld $(BUILD_FOLDER)/bootstrap.elf -o $(BUILD_FOLDER)/sos-kernel.elf

iso: build grub.cfg
	mkdir -p $(BUILD_FOLDER)/iso/boot/grub
	cp grub.cfg $(BUILD_FOLDER)/iso/boot/grub/grub.cfg
	cp $(BUILD_FOLDER)/sos-kernel.elf $(BUILD_FOLDER)/iso/boot/sos-kernel.elf
	grub-mkrescue -o $(BUILD_FOLDER)/grub.iso $(BUILD_FOLDER)/iso

run: build iso
	qemu-system-x86_64 -cdrom $(BUILD_FOLDER)/grub.iso

clean:
	rm -rf build
