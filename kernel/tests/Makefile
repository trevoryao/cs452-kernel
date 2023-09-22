XDIR:=$(HOME)/xdev
ARCH=cortex-a72
TRIPLE=aarch64-none-elf
XBINDIR:=$(XDIR)/bin
CC:=clang
OBJCOPY:=$(XBINDIR)/$(TRIPLE)-objcopy
OBJDUMP:=$(XBINDIR)/$(TRIPLE)-objdump
ROOT:=../..
SRC:=src
INCLUDES:=include
KDIR:=kernel
UDIR:=user
EXEC:=kernel-test

# COMPILE OPTIONS
# -ffunction-sections causes each function to be in a separate section (linker script relies on this)
WARNINGS=-Wall -Wextra -Wpedantic -Wno-unused-const-variable
CFLAGS:=-g -pipe -static $(WARNINGS) -ffreestanding -nostartfiles -ffunction-sections \
	-mcpu=$(ARCH) -static-pie -mstrict-align -fno-builtin -mgeneral-regs-only \
	-I$(ROOT)/$(INCLUDES) -I$(ROOT)/$(KDIR)/$(INCLUDES) -I$(ROOT)/$(UDIR)/$(INCLUDES)

# -Wl,option tells g++ to pass 'option' to the linker with commas replaced by spaces
# doing this rather than calling the linker ourselves simplifies the compilation procedure
LDFLAGS:=-Wl,-nmagic -Wl,-Tlinker.ld

# Source files and include dirs
SOURCES := $(EXEC).c ../src/task-memory.c ../src/task-queue.c ../src/task-state.c

# Create .o and .d files for every .cc and .S (hand-written assembly) file
OBJECTS := $(patsubst %.c, %.o, $(patsubst %.S, %.o, $(SOURCES)))
DEPENDS := $(patsubst %.c, %.d, $(patsubst %.S, %.d, $(SOURCES)))

# The first rule is the default, ie. "make", "make all" and "make kernel8.img" mean the same
${EXEC} : ${OBJECTS}				# link step
	${CXX} ${CXXFLAGS} $^ -o $@		# additional object files before $^

all: a0.img

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(EXEC)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.o: %.S Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPENDS)
