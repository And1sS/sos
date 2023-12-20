BUILD_FOLDER = $(shell pwd | rev | cut -d'/' -f2- | rev)/build_output/

ISO_FILE = $(BUILD_FOLDER)sos.iso
ISO_GRUB_CFG_FILE = $(BUILD_FOLDER)iso/boot/grub/grub.cfg

KERNEL_ELF_NAME = sos-kernel.elf
KERNEL_OUTPUT_ELF_FILE = $(BUILD_FOLDER)kernel/$(KERNEL_ELF_NAME)
ISO_KERNEL_FOLDER = $(BUILD_FOLDER)iso/boot/
ISO_KERNEL_FILE = $(ISO_KERNEL_FOLDER)$(KERNEL_ELF_NAME)

USER_OUTPUT_FOLDER = $(BUILD_FOLDER)user/

ISO_FOLDER = $(BUILD_FOLDER)iso/

QEMU_FLAGS = -D ./log.txt -d int,cpu_reset -no-reboot

all: iso

run: iso
	qemu-system-x86_64 -cdrom $(ISO_FILE) $(QEMU_FLAGS)

clean:
	rm -rf $(BUILD_FOLDER)

kernel: FORCE
	$(MAKE) BUILD_FOLDER=$(BUILD_FOLDER)kernel -C kernel
	mkdir -p $(ISO_KERNEL_FOLDER)
	cp $(KERNEL_OUTPUT_ELF_FILE) $(ISO_KERNEL_FILE)

iso-user: user
	echo $(shell find $(USER_OUTPUT_FOLDER) -name "*.elf")
	mkdir -p $(ISO_FOLDER)user
	cp -r $(shell find $(USER_OUTPUT_FOLDER) -name "*.elf") $(ISO_FOLDER)user

user: FORCE
	$(MAKE) BUILD_FOLDER=$(USER_OUTPUT_FOLDER) -C user

iso: $(ISO_FILE)

$(ISO_FILE): $(ISO_GRUB_CFG_FILE) iso-user kernel
	grub-mkrescue -o $(ISO_FILE) $(BUILD_FOLDER)iso

$(ISO_GRUB_CFG_FILE): grub.cfg
	mkdir -p $(@D)
	cp $< $@

FORCE:



