ASM=nasm
CC=gcc

SRC_DIR=src
UTIL_DIR=util
BUILD_DIR=build

.PHONY: all floppy_image kernel bootloader clean always util_fat

all: floppy_image util_fat

################
# Floppy image #
################

floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "hOS" $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/bootloader.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	mcopy -i $(BUILD_DIR)/main_floppy.img test.txt "::test.txt"

##############
# Bootloader #
##############

bootloader: $(BUILD_DIR)/bootloader.bin

$(BUILD_DIR)/bootloader.bin: always
	$(ASM) $(SRC_DIR)/bootloader/boot.asm -f bin -o $(BUILD_DIR)/bootloader.bin

##########
# Kernel #
##########

kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: always
	$(ASM) $(SRC_DIR)/kernel/main.asm -f bin -o $(BUILD_DIR)/kernel.bin

########
# Util #
########

util_fat: $(BUILD_DIR)/util/fat
$(BUILD_DIR)/util/fat: always $(UTIL_DIR)/fat/fat.c
	mkdir -p $(BUILD_DIR)/util
	$(CC) -g -o $(BUILD_DIR)/util/fat $(UTIL_DIR)/fat/fat.c

##########
# Always #
##########

always:
	mkdir -p $(BUILD_DIR)

#########
# Clean #
#########

clean:
	rm -rf $(BUILD_DIR)/*
