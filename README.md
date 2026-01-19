# DShot Motor Controller for STM32

A bare-metal DShot600 motor controller with serial ESC telemetry for STM32F4 microcontrollers. Built without vendor HAL libraries or Arduino—just register-level C code.

## Features

- **DShot600 Protocol**: Digital motor control via Timer + DMA
- **Serial ESC Telemetry**: Real-time RPM, voltage, current, temperature, and consumption data (KISS/BLHeli32 protocol)
- **Interactive Control**: Serial terminal interface for throttle adjustment and testing
- **Minimal Dependencies**: No HAL, no RTOS—just CMSIS headers and your toolchain

## Hardware Requirements

- STM32F4 board (tested on STM32F411, adaptable to others)
- ESC with serial telemetry support (BLHeli_32, KISS, etc.)
- Brushless motor
- ST-Link programmer
- USB-to-Serial adapter (for debug UART)

### Pin Configuration (Default)

| Function | Pin | Notes |
|----------|-----|-------|
| DShot Signal | PA8 | TIM1_CH1 output to ESC |
| ESC Telemetry | PA10 | USART1_RX from ESC telemetry wire |
| Debug UART TX | PA2 | USART2 to serial adapter |
| Debug UART RX | PA3 | USART2 from serial adapter |

## Software Requirements

1. **ARM GCC Toolchain**
   ```bash
   # Ubuntu/Debian
   sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi

   # macOS
   brew install --cask gcc-arm-embedded
   ```

2. **OpenOCD** (for flashing)
   ```bash
   # Ubuntu/Debian
   sudo apt install openocd

   # macOS
   brew install openocd
   ```

3. **Make** (build system)

## Project Structure

```
BiDShot/
├── src/
│   ├── main.c              # Application with motor control and UI
│   ├── dshot.c             # DShot protocol (Timer + DMA)
│   ├── esc_telemetry.c     # Serial telemetry reception
│   ├── uart.c              # Debug UART driver
│   ├── nvic.c              # Interrupt controller setup
│   └── system_stm32f4xx.c  # Clock configuration
├── inc/
│   ├── dshot.h             # DShot configuration and API
│   ├── esc_telemetry.h     # Telemetry configuration and API
│   ├── uart.h              # UART API
│   └── stm32f4xx.h         # Register definitions
├── startup/
│   └── startup_stm32f411xe.s
├── linker/
│   └── STM32F411xE.ld
├── Makefile
├── ARCHITECTURE.md         # Technical deep-dive
├── DSHOT_REFERENCE.md      # Protocol documentation
└── SETUP_GUIDE.md          # Detailed setup instructions
```

## Configuration

**DShot settings** (`inc/dshot.h`):
- `DSHOT_SPEED` — Protocol speed (150, 300, 600, 1200)
- `DSHOT_TIMER` — Timer peripheral (TIM1)
- `DSHOT_GPIO_PIN` — Output pin (8 for PA8)

**Telemetry settings** (`inc/esc_telemetry.h`):
- `ESC_TELEM_RX_PIN` — Telemetry input pin (10 for PA10)
- `MOTOR_POLES` — Motor pole pairs (for RPM calculation)

## Building and Flashing

```bash
make          # Build firmware
make flash    # Flash via OpenOCD
make clean    # Clean build artifacts
make size     # Show memory usage
```

## Usage

1. Connect ESC signal wire to PA8
2. Connect ESC telemetry wire to PA10
3. Connect debug UART (PA2 TX) to USB-serial adapter
4. Flash the firmware
5. Open serial terminal at 115200 baud

### Operating Modes

**Interactive Mode** (default): Control motor via serial commands
- `+` / `-` — Increase/decrease throttle by 50
- `0` — Stop motor
- `b` — Beep ESC
- `t` — Run automated test cycle
- `h` — Show help

**Automatic Test Mode**: Cycles through throttle values displaying telemetry

### Telemetry Output

```
[Thr: 548 | RPM: 12450 | 42 C | 14.80V | 3.25A | 127mAh]
```

## Safety

**Remove propellers before testing.** The motor will spin.

- ESC must be properly powered before initialization
- Throttle values 0-47 are reserved for special commands
- Motor commands start at 48 (DSHOT_THROTTLE_MIN)

## Troubleshooting

**No telemetry data:**
- Verify ESC has serial telemetry output (separate from signal wire)
- Check PA10 connection to ESC telemetry pad
- Confirm ESC firmware supports telemetry (BLHeli_32 configurator)

**Motor doesn't spin:**
- Check DShot signal with logic analyzer (1.67μs bit period for DShot600)
- Verify ESC is armed (needs ~1 second of zero throttle)
- Confirm throttle value is ≥48

**Build errors:**
- Ensure `arm-none-eabi-gcc` is in PATH
- Check that startup/ and linker/ directories exist with required files

## Adapting to Other MCUs

1. Replace startup assembly and linker script
2. Adjust clock configuration in `system_stm32f4xx.c`
3. Update timer/DMA/GPIO assignments in header files
4. Modify Makefile MCU flags

See `ARCHITECTURE.md` for detailed hardware abstraction notes.

## License

MIT License
