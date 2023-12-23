RESULT_BINARY = $(BUILD_FOLDER)first-userspace-program.bin

all: $(RESULT_BINARY)

clean:
	rm -rf $(BUILD_FOLDER)

$(RESULT_BINARY): first-userspace-program.asm
	mkdir -p $(BUILD_FOLDER)
	nasm first-userspace-program.asm -f bin -o $(BUILD_FOLDER)first-userspace-program.bin

.PHONY: all
