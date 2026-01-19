# BiDShot - Bidirectional DShot Architecture & Summary

## Project Overview

BiDShot is a complete, from-scratch implementation of the bidirectional DShot protocol for STM32 microcontrollers using VSCode as the development environment. The project demonstrates how to:

1. Send digital motor commands via DShot600
2. Receive telemetry data (eRPM) from ESCs on the **same signal wire**
3. Decode 21-bit GCR-encoded telemetry responses
4. Display telemetry via serial UART
5. Build embedded firmware without Arduino or vendor IDEs

**Key Feature:** Single-wire bidirectional communication—the DShot signal pin (PA8) switches between output mode (sending commands) and input capture mode (receiving telemetry).

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│                    (main.c)                              │
│  - Motor control loop                                    │
│  - User interface (serial commands)                      │
│  - Test sequences                                        │
└────────────────┬────────────────────────────────────────┘
                 │
┌────────────────┴────────────────────────────────────────┐
│                    Driver Layer                          │
│                                                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ DShot Driver │  │ UART Driver  │  │  NVIC/System │  │
│  │  (dshot.c)   │  │  (uart.c)    │  │  (nvic.c)    │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└────────────────┬────────────────────────────────────────┘
                 │
┌────────────────┴────────────────────────────────────────┐
│                 Hardware Layer (STM32)                   │
│                                                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌─────────┐ │
│  │  Timer   │  │   DMA    │  │   GPIO   │  │  USART  │ │
│  │  (TIM1)  │  │ (DMA2_S1)│  │  (GPIOA) │  │(USART2) │ │
│  └──────────┘  └──────────┘  └──────────┘  └─────────┘ │
└─────────────────────────────────────────────────────────┘
```

## File Structure

```
BiDShot/
├── src/                      # Source files
│   ├── main.c               # Main application with motor control
│   ├── dshot.c              # Bidirectional DShot protocol implementation
│   ├── esc_telemetry.c      # Telemetry compatibility layer
│   ├── uart.c               # Serial UART driver
│   ├── nvic.c               # Interrupt controller
│   └── system_stm32f4xx.c   # System initialization
│
├── inc/                      # Header files
│   ├── dshot.h              # Bidirectional DShot API and configuration
│   ├── esc_telemetry.h      # Telemetry interface
│   ├── uart.h               # UART API
│   └── stm32f4xx.h          # Register definitions
│
├── startup/                  # Startup code
│   └── startup_stm32f411xe.s # ARM assembly startup
│
├── linker/                   # Linker scripts
│   └── STM32F411xE.ld       # Memory layout
│
├── .vscode/                  # VSCode configuration
│   ├── tasks.json           # Build tasks
│   └── c_cpp_properties.json # IntelliSense config
│
├── build/                    # Build output (generated)
│
├── Makefile                  # Build system
├── setup.sh                  # Helper setup script
│
├── README.md                 # Project documentation
├── ARCHITECTURE.md          # This file - technical deep-dive
├── SETUP_GUIDE.md           # Detailed setup instructions
└── DSHOT_REFERENCE.md       # DShot protocol reference
```

## Key Components

### 1. Bidirectional DShot Driver (dshot.c/h)

**Responsibilities:**
- DShot frame generation with CRC
- Timer and DMA configuration for PWM output
- **Bidirectional GPIO switching** (output → input → output)
- Input capture for telemetry response
- **GCR decoding** of 21-bit ESC responses
- State machine management (IDLE → SENDING → WAIT_TELEM → RECEIVING → PROCESSING)

**Key Functions:**
- `dshot_init()` - Initialize hardware for bidirectional operation
- `dshot_send_throttle()` - Send motor command (telemetry always requested)
- `dshot_get_telemetry()` - Read eRPM/RPM data
- `dshot_send_command()` - Send special commands (beeps, direction, etc.)
- `dshot_update()` - Process telemetry state machine (call from main loop)

**Hardware Used:**
- TIM1 Channel 1 (configurable) - PWM output and input capture
- DMA2 Stream 1 - Output DMA for PWM duty cycles
- DMA2 Stream 6 - Input capture DMA for telemetry edges
- GPIO PA8 - **Bidirectional** (switches between output and input modes)

### 2. UART Driver (uart.c/h)

**Responsibilities:**
- Serial communication at 115200 baud
- Printf-like formatted output
- Simple character I/O

**Key Functions:**
- `uart_init()` - Initialize UART
- `uart_printf()` - Formatted output
- `uart_puts()` - String output

**Hardware Used:**
- USART2
- GPIO PA2 (TX), PA3 (RX)

### 3. Main Application (main.c)

**Responsibilities:**
- System clock initialization
- ESC arming sequence
- Motor control loops
- Interactive user interface
- Test sequences

**Modes:**
1. **Automatic Test Cycle**: Ramps through throttle values
2. **Interactive Mode**: User controls via serial commands

**Features:**
- Real-time RPM display
- Safe arming sequence
- Graceful shutdown
- Error handling

## Hardware Requirements

### Minimum Configuration
- STM32F4 (F411 or higher recommended)
- 168 MHz timer clock
- 1 advanced timer (TIM1) or general-purpose timer
- 1 DMA stream
- 1 UART for debugging
- ST-Link programmer

### Recommended Hardware
- STM32F411 Nucleo/BlackPill board
- ESC with bidirectional DShot support (BLHeli_32, etc.)
- Brushless motor (remove propeller!)
- USB-to-Serial adapter
- Logic analyzer (for debugging)
- Oscilloscope (optional, for timing verification)

## Software Requirements

1. **ARM GCC Toolchain** (gcc-arm-none-eabi)
2. **OpenOCD** or ST-Link tools
3. **VSCode** with C/C++ extension
4. **Make** (build system)
5. **Serial terminal** (screen, minicom, PuTTY)

## Protocol Implementation Details

### DShot Frame Transmission

1. **Encode**: Convert 16-bit DShot frame to PWM duty cycles
2. **DMA**: Transfer duty values to timer CCR register
3. **Timer**: Generate PWM output automatically
4. **Complete**: DMA interrupt fires when done

### Bidirectional Telemetry Reception

1. **Switch**: After DMA complete interrupt, reconfigure PA8 from output to input capture
2. **Wait**: ESC responds ~25-30μs after receiving the command frame
3. **Capture**: Use input capture with DMA to record all edge timings
4. **Decode**: Convert pulse widths to bits (response is at 5/4 command bitrate = 750kbps for DShot600)
5. **GCR Decode**: Convert 21-bit GCR-encoded response to 16-bit data (12-bit period + 4-bit CRC)
6. **Validate**: Check CRC and convert period to eRPM/RPM
7. **Restore**: Switch PA8 back to output mode for next command

### Timing (DShot600)

**Command (STM32 → ESC):**
- **Bit period**: 1.67 μs
- **Frame time**: ~27 μs (16 bits)
- **'0' bit duty**: 37.5% (625 ns high)
- **'1' bit duty**: 75% (1250 ns high)

**Telemetry Response (ESC → STM32):**
- **Response delay**: ~25-30 μs after command frame ends
- **Response bitrate**: 750 kbps (5/4 × command rate)
- **Response bit period**: ~1.33 μs
- **Response frame**: 21 bits GCR-encoded (~28 μs)

**Update rate**: 50 Hz (20 ms between frames)

## Configuration Options

All configurable in `inc/dshot.h`:

```c
#define DSHOT_SPEED             600    // 150, 300, 600, 1200
#define MOTOR_POLES             14     // Your motor pole pairs (for RPM calculation)
#define DSHOT_TIMER             TIM1   // Timer selection (must support DMA)
#define DSHOT_GPIO_PORT         GPIOA  // GPIO port
#define DSHOT_GPIO_PIN          8      // Pin number (PA8 for TIM1_CH1)
#define DSHOT_DMA_STREAM        DMA2_Stream1  // DMA for PWM output
#define DSHOT_IC_DMA_STREAM     DMA2_Stream6  // DMA for input capture
```

**Note:** The GPIO pin must support both timer output compare (for sending) and input capture (for receiving telemetry).



## Build Commands
```bash
make           # Build project
make clean     # Clean build files
make flash     # Flash via OpenOCD
make size      # Show memory usage
make disasm    # Generate disassembly
```

## Safety Features

1. **Safe initialization**: Motors start disarmed
2. **Arming sequence**: Required before motor spins
3. **Watchdog ready**: Can add WDT for failsafe
4. **Graceful shutdown**: Ramps down before stopping
5. **Command validation**: Throttle range checks
6. **Error handling**: Invalid telemetry ignored

## Usage Examples

### Basic Usage
```c
// Initialize
dshot_init();
uart_init(115200);

// Arm ESC (send zero throttle for ~1 second)
for (int i = 0; i < 100; i++) {
    dshot_send_throttle(DSHOT_THROTTLE_MIN);  // 48
    dshot_update();  // Process telemetry state machine
    delay_ms(10);
}

// Control motor (telemetry is always requested in bidirectional mode)
dshot_send_throttle(500);

// Process and read telemetry
dshot_update();
if (dshot_telemetry_available()) {
    dshot_telemetry_t* telem = dshot_get_telemetry();
    printf("RPM: %u (eRPM: %u)\r\n", telem->rpm, telem->erpm);
}
```

### Interactive Commands
- `+` : Increase throttle
- `-` : Decrease throttle
- `0` : Stop motor
- `b` : Beep ESC
- `t` : Run test cycle
- `h` : Show help



## License

MIT License - Free to use, modify, and distribute.


## Credits

This implementation is based on:
- DShot protocol specification
- Betaflight open-source flight controller
- STM32 reference designs
- Community knowledge sharing

---

**Version**: 1.0  
**Date**: January 2026  
**Author**: Custom implementation for educational purposes  
**Target**: STM32F4 series (adaptable to others)
