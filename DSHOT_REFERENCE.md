# DShot Protocol Quick Reference

## Protocol Overview

DShot (Digital Shot) is a digital communication protocol for ESCs that provides advantages over analog protocols like PWM or Oneshot125.

### Key Benefits
- Digital communication (no calibration needed)
- Built-in CRC for error detection
- Bidirectional telemetry (eRPM, temperature, voltage, current)
- Special commands (beeps, direction reversal, 3D mode)
- Faster update rates

## DShot Variants

| Variant | Bit Rate | Bit Time | Frame Time | Use Case |
|---------|----------|----------|------------|----------|
| DShot150 | 150 kbit/s | 6.67 μs | ~107 μs | Low-end ESCs |
| DShot300 | 300 kbit/s | 3.33 μs | ~53 μs | Standard |
| DShot600 | 600 kbit/s | 1.67 μs | ~27 μs | High performance |
| DShot1200 | 1200 kbit/s | 0.83 μs | ~13 μs | Racing |

## Frame Structure

```
Total: 16 bits
┌─────────────┬─┬──────┐
│ Throttle    │T│ CRC  │
│ 11 bits     │1│ 4    │
└─────────────┴─┴──────┘

Throttle: 0-2047 (0-47 reserved for commands)
T: Telemetry request bit (0 or 1)
CRC: 4-bit checksum
```

### Bit Encoding
- **'0' bit**: ~37.5% duty cycle (shorter pulse)
- **'1' bit**: ~75% duty cycle (longer pulse)

For DShot600:
- 0 = 625ns HIGH, 1042ns LOW
- 1 = 1250ns HIGH, 417ns LOW

## Throttle Values

| Range | Meaning |
|-------|---------|
| 0-47 | Special commands |
| 48 | Motor stopped/disarmed |
| 49-2047 | Throttle (49 = minimum, 2047 = maximum) |

## Special Commands (0-47)

| Command | Value | Description |
|---------|-------|-------------|
| MOTOR_STOP | 0 | Disarm/stop motor |
| BEEP1 | 1 | Short beep |
| BEEP2 | 2 | Medium beep |
| BEEP3 | 3 | Long beep |
| BEEP4 | 4 | Longer beep |
| BEEP5 | 5 | Longest beep |
| ESC_INFO | 6 | Request ESC info |
| SPIN_DIR_1 | 7 | Set normal direction |
| SPIN_DIR_2 | 8 | Set reversed direction |
| 3D_MODE_OFF | 9 | Disable 3D mode |
| 3D_MODE_ON | 10 | Enable 3D mode |
| SETTINGS_REQ | 11 | Request settings |
| SAVE_SETTINGS | 12 | Save settings to EEPROM |

**Note:** Commands must be sent repeatedly (typically 6-10 times) to take effect.

## CRC Calculation

```c
uint16_t packet = (throttle << 1) | telemetry_bit;
uint16_t crc = (packet ^ (packet >> 4) ^ (packet >> 8)) & 0x0F;
uint16_t frame = (packet << 4) | crc;
```

## Bidirectional Telemetry

### Timing
- ESC responds ~30μs after receiving DShot frame
- Response is 21 bits long (GCR encoded)
- Switch GPIO to input capture mode after transmission

### GCR Encoding
- 21 bits encode 16 bits of data
- Every 5 bits represent 4 bits of data
- Provides DC balance and error detection

### Telemetry Data (16 bits)
```
┌──────────────┬──────┐
│ eRPM         │ CRC  │
│ 12 bits      │ 4    │
└──────────────┴──────┘
```

### eRPM to RPM Conversion
```
RPM = (eRPM * 100) / motor_pole_pairs
```

Example: 
- eRPM = 1000
- Motor poles = 14
- RPM = (1000 * 100) / 14 = 7142 RPM

## Implementation Checklist

### Hardware Requirements
- [ ] Timer with DMA support
- [ ] GPIO pin with both output and input capture
- [ ] Timer frequency ≥ 168 MHz (for DShot600)
- [ ] DMA channel available

### Software Requirements
- [ ] Timer configuration (PWM mode)
- [ ] DMA configuration (memory to peripheral)
- [ ] GPIO alternate function setup
- [ ] DMA complete interrupt handler
- [ ] Input capture interrupt handler (for telemetry)
- [ ] GCR decoder
- [ ] CRC validation

### Testing Steps
1. [ ] Verify DShot output with logic analyzer
2. [ ] Send command 0 (motor stop) repeatedly
3. [ ] Send beep commands to confirm ESC response
4. [ ] Gradually increase throttle from 48
5. [ ] Request and verify telemetry data
6. [ ] Test at various throttle levels

## Common Issues

### Motor doesn't respond
- Check signal wire connection
- Verify ESC is powered
- Ensure throttle ≥ 48
- Send beep command to verify communication
- Check DShot timing with oscilloscope

### Telemetry not received
- ESC must support bidirectional DShot
- Motor must be spinning
- GPIO must switch to input after transmission
- Check input capture configuration
- Verify GCR decoding logic

### Erratic behavior
- Check power supply is stable
- Verify CRC calculation
- Ensure timer frequency is correct
- Check for DMA conflicts
- Verify bit timing accuracy

## Timing Requirements

### DShot600 Example (at 168 MHz timer)
```
Bit period = 1.67 μs
Timer period = 168MHz × 1.67μs = 280 counts

'0' bit = 37.5% duty = 105 counts
'1' bit = 75% duty = 210 counts
```

### Critical Timing
- Bit accuracy: ±5% maximum
- Frame spacing: ≥2 μs between frames
- Telemetry delay: ~30-50 μs after frame
- Update rate: 1-8 kHz (typically 1-2 kHz)

## Protocol Advantages

1. **No calibration**: Digital protocol, no ESC calibration needed
2. **Error detection**: CRC catches transmission errors
3. **Bidirectional**: Read telemetry without extra wires
4. **Fast**: Much faster than traditional PWM (50 Hz)
5. **Robust**: Digital signal less susceptible to noise
6. **Commands**: Special commands for beeps, direction, etc.

## References

- Betaflight DShot implementation: https://github.com/betaflight/betaflight
- BLHeli_32 ESC firmware: https://github.com/bitdump/BLHeli
- DShot specification discussions: https://blck.mn/dshot

## Example Code Snippets

### Sending Throttle
```c
// Send 50% throttle with telemetry request
uint16_t throttle = 1024;  // Mid-range
dshot_send_throttle(throttle, true);
```

### Reading Telemetry
```c
if (dshot_telemetry_available()) {
    dshot_telemetry_t* telem = dshot_get_telemetry();
    printf("RPM: %u\r\n", telem->rpm);
}
```

### Sending Commands
```c
// Make ESC beep
for (int i = 0; i < 10; i++) {
    dshot_send_command(DSHOT_CMD_BEEP1);
    delay_ms(10);
}
```

### Emergency Stop
```c
// Immediately stop motor
dshot_send_throttle(DSHOT_THROTTLE_MIN, false);
// Or use command
dshot_send_command(DSHOT_CMD_MOTOR_STOP);
```
