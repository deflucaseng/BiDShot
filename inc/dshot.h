/**
 * @file dshot.h
 * @brief DShot protocol implementation (transmission only)
 *
 * DShot is a digital ESC protocol that sends 16-bit frames:
 * - 11 bits: throttle value (0-2047)
 * - 1 bit: telemetry request flag (unused, we use serial telemetry)
 * - 4 bits: CRC checksum
 *
 * Telemetry is received separately via ESC serial telemetry (UART).
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

/* Timing calculations for DShot600 at 168MHz timer clock */
#define TIMER_CLOCK_HZ          168000000UL
#define DSHOT_BIT_TIME_US       (1000000UL / (DSHOT_SPEED * 1000UL))
#define DSHOT_TIMER_PERIOD      ((TIMER_CLOCK_HZ * DSHOT_BIT_TIME_US) / 1000000UL)
#define DSHOT_BIT_0_DUTY        (DSHOT_TIMER_PERIOD * 37 / 100)  /* 37.5% duty for '0' */
#define DSHOT_BIT_1_DUTY        (DSHOT_TIMER_PERIOD * 75 / 100)  /* 75% duty for '1' */

/**
 * @brief DShot state machine states
 */
typedef enum {
    DSHOT_STATE_IDLE,
    DSHOT_STATE_SENDING
} dshot_state_t;

/**
 * @brief Initialize DShot protocol
 * @return true if successful, false otherwise
 */
bool dshot_init(void);

/**
 * @brief Send throttle command to ESC
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

#endif /* DSHOT_H */
