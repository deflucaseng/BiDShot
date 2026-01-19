##########################################################
# DShot STM32 Project Makefile
##########################################################

# Project name
PROJECT = dshot

# Target MCU
MCU = cortex-m4
DEVICE = STM32F411xE

# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size
GDB = $(PREFIX)gdb

# Directories
SRC_DIR = src
INC_DIR = inc
BUILD_DIR = build
STARTUP_DIR = startup
LINKER_DIR = linker

# Source files
C_SOURCES = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/dshot.c \
	$(SRC_DIR)/esc_telemetry.c \
	$(SRC_DIR)/uart.c \
	$(SRC_DIR)/nvic.c \
	$(SRC_DIR)/system_stm32f4xx.c

ASM_SOURCES = \
	$(STARTUP_DIR)/startup_stm32f411xe.s

# Include paths
INCLUDES = \
	-I$(INC_DIR) \
	-ICMSIS/Include \
	-ICMSIS/Device/ST/STM32F4xx/Include

# Compiler flags
CFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=soft
CFLAGS += -O2 -g3
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -D$(DEVICE)
CFLAGS += $(INCLUDES)

# Assembler flags
ASFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=soft

# Linker flags
LDFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=soft
LDFLAGS += -T$(LINKER_DIR)/STM32F411xE.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map
LDFLAGS += --specs=nano.specs
LDFLAGS += --specs=nosys.specs

# Object files
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

# Default target
all: $(BUILD_DIR) $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin
	@echo "Build complete!"
	@$(SIZE) $(BUILD_DIR)/$(PROJECT).elf

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Assemble startup file
$(BUILD_DIR)/%.o: $(STARTUP_DIR)/%.s
	@echo "Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

# Link
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS)
	@echo "Linking..."
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# Generate hex file
$(BUILD_DIR)/$(PROJECT).hex: $(BUILD_DIR)/$(PROJECT).elf
	@echo "Creating hex file..."
	@$(OBJCOPY) -O ihex $< $@

# Generate binary file
$(BUILD_DIR)/$(PROJECT).bin: $(BUILD_DIR)/$(PROJECT).elf
	@echo "Creating binary file..."
	@$(OBJCOPY) -O binary $< $@

# Flash using OpenOCD
flash: all
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(BUILD_DIR)/$(PROJECT).elf verify reset exit"

# Flash using st-flash (if you have stlink tools)
flash-stlink: all
	st-flash write $(BUILD_DIR)/$(PROJECT).bin 0x8000000

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)

# Debug with GDB
debug:
	$(GDB) $(BUILD_DIR)/$(PROJECT).elf

# Disassemble
disasm: $(BUILD_DIR)/$(PROJECT).elf
	@$(OBJDUMP) -d $< > $(BUILD_DIR)/$(PROJECT).disasm
	@echo "Disassembly saved to $(BUILD_DIR)/$(PROJECT).disasm"

# Show size information
size: $(BUILD_DIR)/$(PROJECT).elf
	@$(SIZE) $<

.PHONY: all clean flash flash-stlink debug disasm size
