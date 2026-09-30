#
# EN7523 open-source BL1 - standalone flat-binary build
#
# Produces bl1.bin, a drop-in replacement for the vendor blob
# plat/ecnt/blobs/en7523/bl1.bin (<=2KB, AArch32/ARMv7-A, linked at 0x0).
#
# SPDX-License-Identifier: BSD-3-Clause
#

CROSS_COMPILE	?= arm-linux-gnueabihf-
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)ld
OBJCOPY		:= $(CROSS_COMPILE)objcopy
OBJDUMP		:= $(CROSS_COMPILE)objdump

ARCH_FLAGS	:= -mcpu=cortex-a53 -marm -mno-unaligned-access

CFLAGS		:= $(ARCH_FLAGS) -Os -Wall -Wextra \
		   -ffreestanding -fno-builtin -fno-stack-protector \
		   -fno-pic -fno-pie -ffunction-sections -fdata-sections
ASFLAGS		:= $(ARCH_FLAGS)
LDFLAGS		:= -nostdlib -T bl1.ld --gc-sections -Map=bl1.map

OBJS		:= bl1_entry.o bl1_main.o

VENDOR_BLOB	:= ../blobs/en7523/bl1.bin
MAX_SIZE	:= 2048

.PHONY: all clean size dump compare

all: bl1.bin

bl1.bin: bl1.elf
	$(OBJCOPY) -O binary $< $@
	@sz=$$(stat -c%s $@); \
	 echo "  bl1.bin: $$sz bytes (BootROM window limit $(MAX_SIZE))"; \
	 if [ $$sz -gt $(MAX_SIZE) ]; then \
	     echo "  ERROR: image exceeds the 2KB BootROM window"; exit 1; \
	 fi

bl1.elf: $(OBJS) bl1.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

bl1_entry.o: bl1_entry.S
	$(CC) $(ASFLAGS) -c $< -o $@

bl1_main.o: bl1_main.c
	$(CC) $(CFLAGS) -c $< -o $@

size: bl1.elf
	$(CROSS_COMPILE)size bl1.elf

dump: bl1.elf
	$(OBJDUMP) -d bl1.elf

# structural comparison against the vendor blob (not byte-identical)
compare: bl1.bin
	@echo "== open BL1 =="; xxd -e -g4 bl1.bin | head -80
	@echo "== vendor blob =="; xxd -e -g4 $(VENDOR_BLOB) | head -80

clean:
	rm -f $(OBJS) bl1.elf bl1.bin bl1.map
