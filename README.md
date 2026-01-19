# BiDShot - Bidirectional DShot Motor Controller for STM32

A bare-metal bidirectional DShot600 motor controller for STM32F4 microcontrollers. Features single-wire telemetry—the ESC responds with RPM data on the same signal wire used for commands. Built without vendor HAL libraries or Arduino—just register-level C code.

## Features

- **Bidirectional DShot600**: Digital motor control with telemetry on a single wire
- **Single-Wire Telemetry**: Real-time eRPM data received on the signal wire (no separate telemetry wire needed)
- **GCR Decoding**: Automatic decoding of 21-bit GCR-encoded ESC responses
- **Interactive Control**: Serial terminal interface for throttle adjustment and testing
- **Minimal Dependencies**: No HAL, no RTOS—just CMSIS headers and your toolchain

## Hardware Requirements

- STM32F4 board (tested on STM32F411, adaptable to others)
- ESC with bidirectional DShot support (BLHeli_32 with bidir enabled, etc.)
- Brushless motor
- ST-Link programmer
- USB-to-Serial adapter (for debug UART)

### Pin Configuration (Default)

| Function | Pin | Notes |
|----------|-----|-------|
| DShot Signal + Telemetry | PA8 | TIM1_CH1 - bidirectional (output for commands, input for telemetry) |
| Debug UART TX | PA2 | USART2 to serial adapter |
| Debug UART RX | PA3 | USART2 from serial adapter |

**Note:** With bidirectional DShot, telemetry is received on the same PA8 pin used for the DShot signal. No separate telemetry wire is required.

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
- `DSHOT_GPIO_PIN` — Bidirectional signal pin (8 for PA8)
- `MOTOR_POLES` — Motor pole pairs (for RPM calculation)

**Telemetry notes** (`inc/esc_telemetry.h`):
- With bidirectional DShot, telemetry is received on the same pin as the DShot signal
- Only eRPM data is available (no voltage/current/temperature)
- For full telemetry, use ESCs that support Extended DShot Telemetry (EDT)

## Building and Flashing

```bash
make          # Build firmware
make flash    # Flash via OpenOCD
make clean    # Clean build artifacts
make size     # Show memory usage
```

## Usage

1. Connect ESC signal wire to PA8 (this single wire handles both commands and telemetry)
2. Connect debug UART (PA2 TX) to USB-serial adapter
3. Flash the firmware
4. Open serial terminal at 115200 baud
5. Enable bidirectional DShot in your ESC configurator (e.g., BLHeli_32 Configurator)

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
[Thr: 548 | RPM: 12450 | eRPM: 87150]
```

**Note:** Bidirectional DShot only provides RPM data. For voltage, current, and temperature, use ESCs with Extended DShot Telemetry (EDT) or a separate serial telemetry wire.

## Safety

**Remove propellers before testing.** The motor will spin.

- ESC must be properly powered before initialization
- Throttle values 0-47 are reserved for special commands
- Motor commands start at 48 (DSHOT_THROTTLE_MIN)

## Troubleshooting

**No telemetry data:**
- Enable bidirectional DShot in ESC configurator (BLHeli_32: set "Bidirectional DShot" to ON)
- Motor must be spinning to receive telemetry
- Check that PA8 is correctly configured for both output and input capture
- Verify GCR decoding is working (check statistics with 's' command)

**Motor doesn't spin:**
- Check DShot signal with logic analyzer (1.67μs bit period for DShot600)
- Verify ESC is armed (needs ~1 second of zero throttle)
- Confirm throttle value is ≥48
- Send beep command ('b') to verify ESC communication

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
