CC      = gcc
AS      = nasm
LD      = ld
CFLAGS  = -m32 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector -fno-builtin -nostdlib -Icode/include

ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld
BUILD_DIR = build
ISO_ROOT  = iso
ISO_BOOT  = $(ISO_ROOT)/boot
TARGET    = $(BUILD_DIR)/kernel.elf
ISO       = $(BUILD_DIR)/PXK2.iso
DISK      = $(BUILD_DIR)/disk.img
DISK_SIZE = 64

C_SOURCES   := $(shell find code -name '*.c')
ASM_SOURCES := $(shell find code -name '*.s')
OBJS := $(patsubst code/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) $(patsubst code/%.s, $(BUILD_DIR)/%.o, $(ASM_SOURCES))

all: $(ISO) $(DISK)

$(BUILD_DIR)/%.o: code/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: code/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

$(ISO): $(TARGET)
	@mkdir -p $(ISO_BOOT)/grub
	cp $(TARGET) $(ISO_BOOT)/kernel.elf
	grub-mkrescue -o $@ $(ISO_ROOT)

$(DISK):
	@mkdir -p $(BUILD_DIR)
	dd if=/dev/zero of=$(DISK) bs=1M count=$(DISK_SIZE)
	mkfs.ext2 $(DISK)

run: $(ISO) $(DISK)
	qemu-system-i386 \
		-cdrom $(ISO) \
		-drive file=$(DISK),format=raw,index=0,media=disk \
		-m 32 \
		-serial stdio

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(ISO_BOOT)/kernel.elf

clean-disk:
	rm -f $(DISK)

.PHONY: all run clean clean-disk