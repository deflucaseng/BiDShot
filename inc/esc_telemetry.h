/**
 * @file esc_telemetry.h
 * @brief ESC Serial Telemetry reception (KISS/BLHeli32 protocol)
 *
 * Receives telemetry data from ESC via dedicated UART line.
 * Protocol: 115200 baud, 10-byte packets
 *
 * Packet format:
 *   Byte 0:   Temperature (°C)
 *   Byte 1-2: Voltage (0.01V resolution, big-endian)
 *   Byte 3-4: Current (0.01A resolution, big-endian)
 *   Byte 5-6: Consumption (mAh, big-endian)
 *   Byte 7-8: eRPM / 100 (big-endian)
 *   Byte 9:   CRC8
 */

#ifndef ESC_TELEMETRY_H
#define ESC_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>

/* Hardware Configuration - ADJUST FOR YOUR BOARD */
#define ESC_TELEM_USART         USART1
#define ESC_TELEM_USART_RCC     RCC_APB2ENR_USART1EN
#define ESC_TELEM_GPIO_PORT     GPIOA
#define ESC_TELEM_RX_PIN        10      /* PA10 for USART1_RX */
#define ESC_TELEM_GPIO_AF       7       /* AF7 for USART1 */
#define ESC_TELEM_BAUDRATE      115200

/* Telemetry Protocol Constants */
#define ESC_TELEM_PACKET_SIZE   10
#define ESC_TELEM_TIMEOUT_MS    100     /* Timeout for incomplete packets */

/* Motor Configuration */
#define MOTOR_POLES             14      /* Motor pole pairs (adjust for your motor) */

/**
 * @brief ESC telemetry data structure
 */
typedef struct {
    uint8_t  temperature;       /* Temperature in °C */
    uint16_t voltage;           /* Voltage in 0.01V units (e.g., 1480 = 14.80V) */
    uint16_t current;           /* Current in 0.01A units (e.g., 1250 = 12.50A) */
    uint16_t consumption;       /* Consumption in mAh */
    uint16_t erpm;              /* Electrical RPM / 100 */
    uint32_t rpm;               /* Actual RPM (calculated from eRPM and poles) */
    bool     valid;             /* Data validity flag */
    uint32_t last_update;       /* Timestamp of last valid packet */
} esc_telemetry_t;

/**
 * @brief Initialize ESC telemetry reception
 * @return true if successful, false otherwise
 */
bool esc_telemetry_init(void);

/**
 * @brief Process incoming telemetry data (call frequently from main loop)
 *
 * This function checks for incoming bytes and parses complete packets.
 * Should be called regularly to avoid buffer overflow.
 */
void esc_telemetry_update(void);

/**
 * @brief Increment internal tick counter (call from main loop, ~1ms interval)
 *
 * Used for timeout detection on incomplete packets.
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
 * @return Voltage in volts (e.g., 14.80)
 */
float esc_telemetry_get_voltage_v(void);

/**
 * @brief Get current as float in amps
 * @return Current in amps (e.g., 12.50)
 */
float esc_telemetry_get_current_a(void);

#endif /* ESC_TELEMETRY_H */
