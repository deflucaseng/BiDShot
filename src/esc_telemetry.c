/**
 * @file esc_telemetry.c
 * @brief ESC Telemetry wrapper for Bidirectional DShot
 *
 * This module wraps the bidirectional DShot telemetry to provide
 * a compatible API with the original serial telemetry interface.
 */

#include "esc_telemetry.h"
#include "dshot.h"

/* Local telemetry data structure for API compatibility */
static esc_telemetry_t local_telemetry = {0};

/**
 * @brief Initialize ESC telemetry
 *
 * For bidirectional DShot, initialization is handled by dshot_init().
 * This function exists for API compatibility and always returns true.
 */
bool esc_telemetry_init(void) {
    /* Bidirectional DShot telemetry is initialized in dshot_init() */
    local_telemetry.valid = false;
    local_telemetry.temperature = 0;
    local_telemetry.voltage = 0;
    local_telemetry.current = 0;
    local_telemetry.consumption = 0;
    return true;
}

/**
 * @brief Process incoming telemetry data
 *
 * This calls dshot_update() to process the bidirectional telemetry
 * state machine and copies data to the local structure.
 */
void esc_telemetry_update(void) {
    /* Process bidirectional DShot telemetry */
    dshot_update();

    /* Copy data from DShot telemetry to local structure */
    dshot_telemetry_t* dshot_telem = dshot_get_telemetry();

    if (dshot_telem->valid) {
        local_telemetry.erpm = dshot_telem->erpm / 100;  /* Convert to erpm/100 format */
        local_telemetry.rpm = dshot_telem->rpm;
        local_telemetry.valid = true;
        local_telemetry.last_update = dshot_telem->last_update;

        /* These are not available with basic bidirectional DShot */
        local_telemetry.temperature = 0;
        local_telemetry.voltage = 0;
        local_telemetry.current = 0;
        local_telemetry.consumption = 0;
    }
}

/**
 * @brief Increment internal tick counter
 *
 * For bidirectional DShot, timing is handled in dshot_update().
 * This function exists for API compatibility.
 */
void esc_telemetry_tick(void) {
    /* Timing handled internally by dshot_update() */
}

/**
 * @brief Get the latest telemetry data
 */
esc_telemetry_t* esc_telemetry_get(void) {
    return &local_telemetry;
}

/**
 * @brief Check if new telemetry data is available since last call
 */
bool esc_telemetry_available(void) {
    return dshot_telemetry_available();
}

/**
 * @brief Get voltage as float in volts
 * @return 0.0 (not available with basic bidirectional DShot)
 */
float esc_telemetry_get_voltage_v(void) {
    /* Not available with basic bidirectional DShot */
    return 0.0f;
}

/**
 * @brief Get current as float in amps
 * @return 0.0 (not available with basic bidirectional DShot)
 */
float esc_telemetry_get_current_a(void) {
    /* Not available with basic bidirectional DShot */
    return 0.0f;
}
