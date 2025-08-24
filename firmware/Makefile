PROJECT = firmware
CHIP = stm32f4
OPENCM3_DIR = ../libopencm3

# Output files
BINARY = $(PROJECT).elf
MAP    = $(PROJECT).map
HEX    = $(PROJECT).hex
BIN    = $(PROJECT).bin

# Source folders
SRCDIR = src
INCDIR = inc
SHARED_SRCDIR = ../shared/src
SHARED_INCDIR = ../shared/inc

# Find all .c and .S source files
C_SRCS = $(wildcard $(SRCDIR)/**/*.c) $(wildcard $(SRCDIR)/*.c) \
         $(wildcard $(SHARED_SRCDIR)/**/*.c) $(wildcard $(SHARED_SRCDIR)/*.c)
S_SRCS = $(wildcard $(SRCDIR)/**/*.S) $(wildcard $(SRCDIR)/*.S) \
         $(wildcard $(SHARED_SRCDIR)/**/*.S) $(wildcard $(SHARED_SRCDIR)/*.S)

STARTUP = startup.c
SRC = $(C_SRCS) $(S_SRCS) $(STARTUP)
OBJS = $(SRC:.c=.o)
OBJS := $(OBJS:.S=.o)

# Compiler & flags
CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc
CFLAGS = -g -O0 -Wall -Wextra \
         -I$(OPENCM3_DIR)/include \
         -I$(INCDIR) \
         -I$(SHARED_INCDIR) \
         -mcpu=cortex-m4 -mthumb -nostdlib -MD -MP \
         -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DSTM32F4

# Linker & flags
LDFLAGS = -Tlinker.ld -nostartfiles \
          -Wl,--gc-sections,--exclude-libs=ALL,-Map=$(MAP) \
          -mcpu=cortex-m4 -mthumb \
          -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
          -L$(OPENCM3_DIR)/lib -lopencm3_$(CHIP)

# Default target
all: $(BINARY) $(HEX) $(BIN)

# Link rule
$(BINARY): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Convert ELF to HEX and BIN
%.hex: %.elf
	arm-none-eabi-objcopy -O ihex $< $@

%.bin: %.elf
	arm-none-eabi-objcopy -O binary $< $@
#	python pad-bootloader.py

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile Assembly files
%.o: %.S
	$(AS) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	del /Q /S *.o *.d $(BINARY) $(HEX) $(BIN) $(MAP) 2>nul || exit 0

# Flash rule (optional)
flash: $(BINARY)
	st-flash write $(BIN) 0x8000000
