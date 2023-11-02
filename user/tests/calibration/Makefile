XDIR:=$(HOME)/xdev
ARCH=cortex-a72
TRIPLE=aarch64-none-elf
XBINDIR:=$(XDIR)/bin
CC:=$(XBINDIR)/$(TRIPLE)-gcc
OBJCOPY:=$(XBINDIR)/$(TRIPLE)-objcopy
OBJDUMP:=$(XBINDIR)/$(TRIPLE)-objdump
ROOT:=../../..
SRC:=src
INCLUDES:=include
KDIR:=kernel
UDIR:=user
EXEC:=calibration-test

# COMPILE OPTIONS
# -ffunction-sections causes each function to be in a separate section (linker script relies on this)
WARNINGS=-Wall -Wextra -Wpedantic -Wno-unused-const-variable
CFLAGS:=-g -pipe -static $(WARNINGS) -ffreestanding -nostartfiles \
	-mcpu=$(ARCH) -static-pie -mstrict-align -fno-builtin -mgeneral-regs-only -O3 \
	-I$(ROOT)/$(INCLUDES) -I$(ROOT)/$(KDIR)/$(INCLUDES) -I$(ROOT)/$(UDIR)/$(INCLUDES)

# -Wl,option tells g++ to pass 'option' to the linker with commas replaced by spaces
# doing this rather than calling the linker ourselves simplifies the compilation procedure
LDFLAGS:=-Wl,-nmagic -Wl,-T$(ROOT)/linker.ld

# Source files and include dirs
SOURCES := $(EXEC).c $(shell find $(ROOT)/$(SRC) $(ROOT)/$(KDIR)/$(SRC) $(ROOT)/$(UDIR)/$(SRC) -type f ! -name main.c -name '*.c' -o -name '*.S')

# Create .o and .d files for every .cc and .S (hand-written assembly) file
OBJECTS := $(patsubst %.c, %.o, $(patsubst %.S, %.o, $(SOURCES)))
DEPENDS := $(patsubst %.c, %.d, $(patsubst %.S, %.d, $(SOURCES)))

# The first rule is the default, ie. "make", "make all" and "make kernel8.img" mean the same
all: $(EXEC).img

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(EXEC).elf $(EXEC).img

$(EXEC).img: $(EXEC).elf
	$(OBJCOPY) $< -O binary $@

$(EXEC).elf: $(OBJECTS) $(ROOT)/linker.ld
	$(CC) $(CFLAGS) $(filter-out %.ld, $^) -o $@ $(LDFLAGS)
	@$(OBJDUMP) -d $(EXEC).elf | fgrep -q q0 && printf "\n***** WARNING: SIMD INSTRUCTIONS DETECTED! *****\n\n" || true

%.o: %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.o: %.S Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
