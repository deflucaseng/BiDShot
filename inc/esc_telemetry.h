/**
 * @file esc_telemetry.h
 * @brief ESC Telemetry interface for Bidirectional DShot
 *
 * This module provides a compatibility layer for telemetry data
 * received via bidirectional DShot on the same signal wire.
 *
 * With bidirectional DShot:
 * - Telemetry is received on PA8 (same pin as DShot output)
 * - No separate UART wire needed
 * - Only eRPM data is available (no voltage/current/temperature)
 *
 * For full telemetry (voltage, current, temp), use ESCs that support
 * the separate serial telemetry wire, or EDT (Extended DShot Telemetry).
 */

#ifndef ESC_TELEMETRY_H
#define ESC_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

/* Motor Configuration */
#define MOTOR_POLES             14      /* Motor pole pairs (adjust for your motor) */

/**
 * @brief ESC telemetry data structure
 *
 * With basic bidirectional DShot, only RPM data is available.
 * Voltage, current, and temperature fields are set to 0.
 */
typedef struct {
    uint8_t  temperature;       /* Temperature in °C (not available in basic bidir) */
    uint16_t voltage;           /* Voltage in 0.01V units (not available in basic bidir) */
    uint16_t current;           /* Current in 0.01A units (not available in basic bidir) */
    uint16_t consumption;       /* Consumption in mAh (not available in basic bidir) */
    uint16_t erpm;              /* Electrical RPM / 100 */
    uint32_t rpm;               /* Actual RPM (calculated from eRPM and poles) */
    bool     valid;             /* Data validity flag */
    uint32_t last_update;       /* Timestamp of last valid packet */
} esc_telemetry_t;

/**
 * @brief Initialize ESC telemetry
 *
 * For bidirectional DShot, this is handled by dshot_init().
 * This function exists for API compatibility.
 *
 * @return true (always succeeds as DShot handles initialization)
 */
bool esc_telemetry_init(void);

/**
 * @brief Process incoming telemetry data
 *
 * For bidirectional DShot, call dshot_update() instead.
 * This function exists for API compatibility.
 */
void esc_telemetry_update(void);

/**
 * @brief Increment internal tick counter
 *
 * For bidirectional DShot, timing is handled internally.
 * This function exists for API compatibility.
 */
void esc_telemetry_tick(void);

/**
 * @brief Get the latest telemetry data
 * @return Pointer to telemetry structure (always valid, check 'valid' flag)
 */
esc_telemetry_t* esc_telemetry_get(void);

/**
 * @brief Check if new telemetry data is available since last call
 * @return true if new data available
 */
bool esc_telemetry_available(void);

/**
 * @brief Get voltage as float in volts
 * @return 0.0 (not available with basic bidirectional DShot)
 */
float esc_telemetry_get_voltage_v(void);

/**
 * @brief Get current as float in amps
 * @return 0.0 (not available with basic bidirectional DShot)
 */
float esc_telemetry_get_current_a(void);

#endif /* ESC_TELEMETRY_H */
