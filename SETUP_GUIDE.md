# VSCode Setup Guide for BiDShot Project

This guide will help you set up your development environment for the BiDShot (Bidirectional DShot) STM32 project using VSCode.

## 1. Install Required Software

### ARM Toolchain

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi gdb-multiarch
```

**macOS:**
```bash
brew install --cask gcc-arm-embedded
brew install open-ocd
```

**Windows:**
1. Download ARM GCC from: https://developer.arm.com/downloads/-/gnu-rm
2. Install and add to PATH
3. Download OpenOCD from: https://github.com/openocd-org/openocd/releases

### OpenOCD (for flashing)

**Linux:**
```bash
sudo apt install openocd
```

**macOS:**
```bash
brew install open-ocd
```

### ST-Link Tools (Alternative to OpenOCD)

**Linux:**
```bash
sudo apt install stlink-tools
```

**macOS:**
```bash
brew install stlink
```

## 2. Install VSCode Extensions

Open VSCode and install these extensions:

1. **C/C++** (by Microsoft) - For C/C++ IntelliSense
2. **Cortex-Debug** (optional) - For debugging STM32
3. **Serial Monitor** (optional) - For viewing UART output

Install via VSCode Extensions marketplace or command:
```bash
code --install-extension ms-vscode.cpptools
code --install-extension marus25.cortex-debug
```

## 3. Get CMSIS Headers

The project needs official CMSIS headers from ST. You have two options:

### Option A: Download from ST

1. Go to: https://www.st.com/en/embedded-software/stm32cubef4.html
2. Download STM32CubeF4
3. Extract and copy these folders to your project:
   - `Drivers/CMSIS/Include/` → `BiDShot/CMSIS/Include/`
   - `Drivers/CMSIS/Device/ST/STM32F4xx/Include/` → `BiDShot/CMSIS/Device/ST/STM32F4xx/Include/`

### Option B: Use Minimal Headers (Quick Start)

The project includes minimal header definitions in `inc/stm32f4xx.h` that are sufficient for basic operation. For production use, we recommend getting the full CMSIS headers.

## 4. Get Startup and Linker Files

### Startup File

Download or create `startup_stm32f411xe.s` for your specific MCU:

From STM32CubeF4:
```
STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s
```

Place in: `BiDShot/startup/`

### Linker Script

From STM32CubeF4 or create based on your MCU's memory:
```
STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/linker/STM32F411xE.ld
```

Place in: `BiDShot/linker/`

## 5. Configure Hardware Settings

Edit `inc/dshot.h` to match your hardware:

```c
// Timer and GPIO configuration
#define DSHOT_TIMER             TIM1
#define DSHOT_GPIO_PORT         GPIOA
#define DSHOT_GPIO_PIN          8       // PA8 for TIM1_CH1

// Motor configuration
#define MOTOR_POLES             14      // Your motor's pole pairs
```

Common timer/pin combinations:
- TIM1_CH1: PA8 (AF1)
- TIM2_CH1: PA0 (AF1)
- TIM3_CH1: PA6 (AF2)
- TIM4_CH1: PB6 (AF2)

## 6. Build the Project

### Using VSCode Tasks

1. Open the project folder in VSCode
2. Press `Ctrl+Shift+B` (or `Cmd+Shift+B` on Mac)
3. Select "Build" from the task list

### Using Terminal

```bash
cd BiDShot
make
```

If successful, you'll see:
```
Build complete!
   text    data     bss     dec     hex filename
   1234     100     200    1534     5fe build/dshot.elf
```

## 7. Flash the Firmware

### Using OpenOCD

Connect your ST-Link and run:
```bash
make flash
```

### Using ST-Link Tools

```bash
make flash-stlink
```

### Troubleshooting Flashing

If you get permission errors on Linux:
```bash
sudo usermod -a -G dialout $USER
sudo chmod 666 /dev/ttyUSB0  # or your device
```

Create udev rules for ST-Link:
```bash
sudo nano /etc/udev/rules.d/99-stlink.rules
```

Add:
```
SUBSYSTEM=="usb", ATTR{idVendor}=="0483", ATTR{idProduct}=="3748", MODE="0666"
```

Then:
```bash
sudo udevadm control --reload-rules
```

## 8. Monitor Serial Output

### Using Screen (Linux/macOS)

```bash
screen /dev/ttyUSB0 115200
```

Exit: `Ctrl+A` then `K`

### Using Minicom (Linux)

```bash
minicom -D /dev/ttyUSB0 -b 115200
```

### Using PuTTY (Windows)

1. Open PuTTY
2. Select "Serial"
3. Set port (e.g., COM3)
4. Set speed: 115200
5. Click "Open"

### Using VSCode Serial Monitor Extension

1. Install "Serial Monitor" extension
2. Click the plug icon in status bar
3. Select your port
4. Set baud rate to 115200

## 9. Hardware Connections

### Power
- Connect ESC power (typically 7-25V)
- Connect GND between STM32 and ESC

### Bidirectional DShot Signal
- Connect PA8 to ESC signal wire (this single wire handles both commands AND telemetry)
- ESC signal wire is usually white or yellow
- **Important:** Enable "Bidirectional DShot" in your ESC configurator (BLHeli_32 Configurator)

### Serial Monitor
- Connect PA2 (UART TX) to USB-serial adapter RX
- Connect GND between STM32 and USB-serial

### Programming
- Connect ST-Link to STM32 SWD pins:
  - SWDIO
  - SWCLK
  - GND
  - 3.3V (optional)

**Note:** Unlike traditional DShot setups, bidirectional DShot does NOT require a separate telemetry wire. All communication happens on the single signal wire (PA8).

## 10. Debugging (Optional)

### Using Cortex-Debug Extension

1. Install Cortex-Debug extension
2. Create `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug (OpenOCD)",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "openocd",
            "cwd": "${workspaceRoot}",
            "executable": "./build/dshot.elf",
            "configFiles": [
                "interface/stlink.cfg",
                "target/stm32f4x.cfg"
            ]
        }
    ]
}
```

3. Press F5 to start debugging

## 11. Customization for Different STM32 Models

### For STM32F103 (Blue Pill):
- Replace startup file: `startup_stm32f103xb.s`
- Replace linker script: `STM32F103xB.ld`
- Update Makefile: `DEVICE = STM32F103xB`
- Adjust clock config in `main.c` (72MHz max)

### For STM32F407 (Discovery):
- Already compatible, just update DEVICE in Makefile
- May need different GPIO pins

### For STM32H7:
- Higher performance, more complex clock setup
- Need H7 CMSIS headers
- Adjust timer frequencies

## 12. Common Issues and Solutions

### Build Fails: "arm-none-eabi-gcc not found"
- Ensure ARM toolchain is installed and in PATH
- Check with: `arm-none-eabi-gcc --version`

### Flash Fails: "Could not find device"
- Check ST-Link connection
- Verify USB cable (must support data, not just power)
- Try: `st-info --probe`

### Motor Doesn't Respond
- Check ESC supports bidirectional DShot
- Verify signal pin connection
- Use logic analyzer to verify DShot timing
- Try different throttle values

### No Serial Output
- Check UART pins (PA2 for TX, PA3 for RX)
- Verify baud rate (115200)
- Check USB-serial adapter connections
- Ensure GND is connected

### Telemetry Not Received
- ESC must support bidirectional DShot (most BLHeli_32 ESCs do)
- **Enable "Bidirectional DShot" in ESC configurator** (this is the most common issue!)
- Check ESC firmware is updated (BLHeli_32 v32.7+ recommended)
- Motor must be spinning to generate telemetry
- Verify timer/DMA configuration for both output and input capture
- Check telemetry statistics with 's' command to see success/error rates

## 13. Next Steps

Once you have the basic system working:
1. Adjust motor control parameters
2. Add more motors (copy timer/DMA config)
3. Implement custom control algorithms
4. Add sensor integration
5. Create custom telemetry handlers

## Safety Reminders

⚠️ **ALWAYS:**
- Remove propellers during testing
- Secure motor before running
- Use appropriate power supply
- Monitor motor temperature
- Start with low throttle values

Happy coding! 🚁
