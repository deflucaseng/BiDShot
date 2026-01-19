# DShot STM32 Project - Architecture & Summary

## Project Overview

This is a complete, from-scratch implementation of the bidirectional DShot protocol for STM32 microcontrollers using VSCode as the development environment. The project demonstrates how to:

1. Send digital motor commands via DShot600
2. Receive telemetry data (eRPM) from ESCs
3. Display telemetry via serial UART
4. Build embedded firmware without Arduino or vendor IDEs

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
dshot-stm32/
├── src/                      # Source files
│   ├── main.c               # Main application with motor control
│   ├── dshot.c              # DShot protocol implementation
│   ├── uart.c               # Serial UART driver
│   ├── nvic.c               # Interrupt controller
│   └── system_stm32f4xx.c   # System initialization
│
├── inc/                      # Header files
│   ├── dshot.h              # DShot API and configuration
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
├── SETUP_GUIDE.md           # Detailed setup instructions
└── DSHOT_REFERENCE.md       # DShot protocol reference
```

## Key Components

### 1. DShot Driver (dshot.c/h)

**Responsibilities:**
- DShot frame generation with CRC
- Timer and DMA configuration
- PWM output for bit encoding
- Input capture for telemetry
- GCR decoding
- State machine management

**Key Functions:**
- `dshot_init()` - Initialize hardware
- `dshot_send_throttle()` - Send motor command
- `dshot_get_telemetry()` - Read eRPM data
- `dshot_send_command()` - Send special commands

**Hardware Used:**
- TIM1 Channel 1 (configurable)
- DMA2 Stream 1
- GPIO PA8 (configurable)

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

1. **Switch**: Reconfigure GPIO from output to input
2. **Capture**: Use input capture to record edge timings
3. **Decode**: Convert pulse widths to bits
4. **GCR**: Decode 21-bit GCR to 16-bit data
5. **Validate**: Check CRC and extract eRPM

### Timing (DShot600)

- **Bit period**: 1.67 μs
- **Frame time**: ~27 μs (16 bits)
- **'0' bit duty**: 37.5% (625 ns high)
- **'1' bit duty**: 75% (1250 ns high)
- **Update rate**: 50 Hz (20 ms between frames)

## Configuration Options

All configurable in `inc/dshot.h`:

```c
#define DSHOT_SPEED             600    // 150, 300, 600, 1200
#define MOTOR_POLES             14     // Your motor poles
#define DSHOT_TIMER             TIM1   // Timer selection
#define DSHOT_GPIO_PORT         GPIOA  // GPIO port
#define DSHOT_GPIO_PIN          8      // Pin number
```

## Build System

### Makefile Features
- Automatic dependency tracking
- Optimized for size (-O2)
- Dead code elimination
- Memory map generation
- Multiple flash options
- Clean rebuild support

### Build Commands
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

// Arm ESC
for (int i = 0; i < 100; i++) {
    dshot_send_throttle(48, false);
    delay_ms(10);
}

// Control motor
dshot_send_throttle(500, true);  // Mid throttle, request telemetry

// Read telemetry
if (dshot_telemetry_available()) {
    dshot_telemetry_t* telem = dshot_get_telemetry();
    printf("RPM: %u\r\n", telem->rpm);
}
```

### Interactive Commands
- `+` : Increase throttle
- `-` : Decrease throttle
- `0` : Stop motor
- `b` : Beep ESC
- `t` : Run test cycle
- `h` : Show help

## Performance

### Memory Usage (Typical)
- Flash: ~2-4 KB
- RAM: ~500 bytes
- Stack: ~1 KB

### CPU Usage
- DShot transmission: <1% @ 50 Hz
- Telemetry processing: <2%
- Main loop: Minimal

### Update Rates
- DShot output: 50-1000 Hz (configurable)
- Telemetry: Every frame (if requested)
- Serial output: As needed

## Extensibility

### Easy to Add
- Multiple motors (copy timer config)
- Different protocols (OneShot, Multishot)
- Additional sensors
- Custom telemetry processing
- Flight controller integration

### Example: Adding a Second Motor
1. Configure TIM2_CH1 in same way as TIM1
2. Copy DShot buffer and state management
3. Add second DMA stream
4. Update interrupt handlers

## Testing Strategy

### Phase 1: Basic Output
1. Build and flash firmware
2. Connect logic analyzer
3. Verify DShot timing
4. Send command 0 (motor stop)

### Phase 2: Motor Control
1. Connect ESC (no propeller!)
2. Send beep commands
3. Gradually increase throttle
4. Verify motor responds

### Phase 3: Telemetry
1. Request telemetry bit
2. Monitor input capture
3. Verify GCR decoding
4. Display RPM values

## Common Customizations

### For STM32F103 (72 MHz)
- Adjust timer calculations
- Use TIM2/TIM3 (no TIM1 advanced features)
- Reduce DShot speed to 300 or 150

### For STM32H7 (High Performance)
- Support DShot1200
- Add oversampling for better precision
- Implement extended telemetry

### For Multiple Motors (Quadcopter)
- Array of DShot structures
- Synchronized updates
- Motor mixing algorithms

## Troubleshooting

See `SETUP_GUIDE.md` for detailed troubleshooting steps.

Quick checks:
1. ✓ ARM toolchain installed?
2. ✓ Correct timer/GPIO pins?
3. ✓ ESC powered properly?
4. ✓ Signal wire connected?
5. ✓ Serial terminal working?

## Future Enhancements

Potential additions:
- [ ] Extended telemetry (temperature, voltage, current)
- [ ] DShot150/300/1200 support
- [ ] Multiple motor support
- [ ] Kalman filtering for smooth RPM
- [ ] Flight controller integration
- [ ] GUI configuration tool
- [ ] ESC passthrough programming
- [ ] Kiss protocol support

## License

MIT License - Free to use, modify, and distribute.

## Resources

- **Betaflight**: Reference implementation
- **BLHeli_32**: ESC firmware with bidirectional DShot
- **STM32 Reference Manual**: Hardware details
- **ARM CMSIS**: Standard peripheral library

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
