/**
 * @file dshot.h
 * @brief Bidirectional DShot protocol implementation
 *
 * DShot is a digital ESC protocol that sends 16-bit frames:
 * - 11 bits: throttle value (0-2047)
 * - 1 bit: telemetry request flag
 * - 4 bits: CRC checksum
 *
 * Bidirectional DShot (inverted DShot) receives telemetry on the same wire:
 * - After sending command, GPIO switches to input
 * - ESC responds with 21-bit GCR-encoded frame at 5/4 the bitrate
 * - Response contains eRPM data (period in microseconds)
 */

#ifndef DSHOT_H
#define DSHOT_H

#include <stdint.h>
#include <stdbool.h>

/* DShot Configuration */
#define DSHOT_SPEED             600     /* DShot600 (can be 150, 300, 600, 1200) */

/* Hardware Configuration - ADJUST FOR YOUR BOARD */
#define DSHOT_TIMER             TIM1
#define DSHOT_TIMER_RCC         RCC_APB2ENR_TIM1EN
#define DSHOT_TIMER_CHANNEL     1       /* Timer channel (1-4) */
#define DSHOT_GPIO_PORT         GPIOA
#define DSHOT_GPIO_PIN          8       /* PA8 for TIM1_CH1 */
#define DSHOT_GPIO_AF           1       /* Alternate function for TIM1 */
#define DSHOT_DMA               DMA2
#define DSHOT_DMA_STREAM        DMA2_Stream1
#define DSHOT_DMA_CHANNEL       6       /* DMA channel for TIM1_CH1 */

/* Input capture DMA (for receiving telemetry) */
#define DSHOT_IC_DMA_STREAM     DMA2_Stream6
#define DSHOT_IC_DMA_CHANNEL    0       /* DMA channel for TIM1_CH1 input capture */

/* DShot Protocol Constants */
#define DSHOT_FRAME_SIZE        16      /* Bits per frame */
#define DSHOT_THROTTLE_MIN      48      /* Minimum throttle (0-47 reserved for commands) */
#define DSHOT_THROTTLE_MAX      2047    /* Maximum throttle */
#define DSHOT_CMD_MAX           47      /* Commands are 0-47 */

/* Special DShot Commands */
#define DSHOT_CMD_MOTOR_STOP    0
#define DSHOT_CMD_BEEP1         1
#define DSHOT_CMD_BEEP2         2
#define DSHOT_CMD_BEEP3         3
#define DSHOT_CMD_BEEP4         4
#define DSHOT_CMD_BEEP5         5
#define DSHOT_CMD_ESC_INFO      6
#define DSHOT_CMD_SPIN_DIR_1    7
#define DSHOT_CMD_SPIN_DIR_2    8
#define DSHOT_CMD_3D_MODE_OFF   9
#define DSHOT_CMD_3D_MODE_ON    10
#define DSHOT_CMD_SETTINGS_REQ  11
#define DSHOT_CMD_SAVE_SETTINGS 12
#define DSHOT_CMD_EXTENDED_TELEM_ENABLE  13
#define DSHOT_CMD_EXTENDED_TELEM_DISABLE 14

/* Bidirectional DShot specific commands (must be sent 6x to take effect) */
#define DSHOT_CMD_BIDIR_EDT_MODE_ON   13
#define DSHOT_CMD_BIDIR_EDT_MODE_OFF  14

/* Timing calculations for DShot600 at 168MHz timer clock */
#define TIMER_CLOCK_HZ          168000000UL
#define DSHOT_BIT_TIME_US       (1000000UL / (DSHOT_SPEED * 1000UL))
#define DSHOT_TIMER_PERIOD      ((TIMER_CLOCK_HZ * DSHOT_BIT_TIME_US) / 1000000UL)
#define DSHOT_BIT_0_DUTY        (DSHOT_TIMER_PERIOD * 37 / 100)  /* 37.5% duty for '0' */
#define DSHOT_BIT_1_DUTY        (DSHOT_TIMER_PERIOD * 75 / 100)  /* 75% duty for '1' */

/* Bidirectional DShot timing
 * ESC responds at 5/4 the command rate
 * For DShot600 (600kbps command) → 750kbps response
 * Response bit time = 0.8 * command bit time
 */
#define DSHOT_TELEM_BITRATE     (DSHOT_SPEED * 1000 * 5 / 4)     /* 750000 for DShot600 */
#define DSHOT_TELEM_BIT_NS      (1000000000UL / DSHOT_TELEM_BITRATE)  /* ~1333ns per bit */

/* GCR (Golay Run Length) encoding for bidirectional telemetry
 * 21 bits total: 20 GCR bits (4 nibbles × 5 bits) + 1 end marker
 * Decodes to 16 bits: 12-bit eRPM period + 4-bit CRC
 */
#define DSHOT_TELEM_FRAME_BITS  21
#define DSHOT_GCR_BITS          5       /* 5 GCR bits per nibble */
#define DSHOT_TELEM_NIBBLES     4       /* 4 nibbles in response */

/* Response timing window (in timer ticks at 168MHz)
 * Wait ~30μs after frame ends before ESC responds
 * Response lasts ~28μs (21 bits at 750kbps)
 */
#define DSHOT_TELEM_DELAY_US    25
#define DSHOT_TELEM_WINDOW_US   50

/* Input capture buffer size (enough for all edges in response) */
#define DSHOT_IC_BUFFER_SIZE    32

/* Motor pole count for RPM calculation */
#define MOTOR_POLES             14

/**
 * @brief DShot state machine states
 */
typedef enum {
    DSHOT_STATE_IDLE,
    DSHOT_STATE_SENDING,
    DSHOT_STATE_WAIT_TELEM,
    DSHOT_STATE_RECEIVING,
    DSHOT_STATE_PROCESSING
} dshot_state_t;

/**
 * @brief Bidirectional telemetry data
 */
typedef struct {
    uint32_t erpm;              /* Electrical RPM */
    uint32_t rpm;               /* Actual RPM (accounting for motor poles) */
    uint16_t period_us;         /* Period in microseconds (raw from ESC) */
    bool     valid;             /* Data validity flag */
    uint32_t last_update;       /* Timestamp of last valid packet */
    uint32_t frame_count;       /* Total frames sent */
    uint32_t success_count;     /* Successful telemetry receptions */
    uint32_t error_count;       /* CRC or decode errors */
} dshot_telemetry_t;

/**
 * @brief Initialize bidirectional DShot protocol
 * @return true if successful, false otherwise
 */
bool dshot_init(void);

/**
 * @brief Send throttle command to ESC with telemetry request
 * @param throttle Throttle value (48-2047, or 0-47 for special commands)
 */
void dshot_send_throttle(uint16_t throttle);

/**
 * @brief Send special DShot command
 * @param command Command value (0-47)
 */
void dshot_send_command(uint8_t command);

/**
 * @brief Check if DShot is ready to send next frame
 * @return true if ready
 */
bool dshot_ready(void);

/**
 * @brief Get current DShot state
 * @return Current state
 */
dshot_state_t dshot_get_state(void);

/**
 * @brief Get telemetry data
 * @return Pointer to telemetry structure
 */
dshot_telemetry_t* dshot_get_telemetry(void);

/**
 * @brief Check if new telemetry is available since last check
 * @return true if new data available
 */
bool dshot_telemetry_available(void);

/**
 * @brief Process bidirectional telemetry (call from main loop)
 *
 * This handles the state machine for receiving and decoding
 * the ESC's telemetry response.
 */
void dshot_update(void);

#endif /* DSHOT_H */
