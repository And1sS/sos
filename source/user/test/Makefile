RESULT_BINARY = $(BUILD_FOLDER)test.bin

CROSS_COMPILE =
CC = gcc
CC_FLAGS = -nostartfiles -nostdlib

LINKER = ld

all: $(RESULT_BINARY)

clean:
	rm -rf $(BUILD_FOLDER)

$(RESULT_BINARY): test.c
	mkdir -p $(BUILD_FOLDER)
	$(CROSS_COMPILE)$(CC) $(CC_FLAGS) test.c -o $(BUILD_FOLDER)test.o
	objcopy -O binary -j .text $(BUILD_FOLDER)test.o $(BUILD_FOLDER)test.bin

.PHONY: all
