/**
 * @file dshot.c
 * @brief Bidirectional DShot protocol implementation
 *
 * Implements DShot600 with bidirectional telemetry on a single wire.
 * After sending the command frame, the GPIO switches to input mode
 * to capture the ESC's GCR-encoded telemetry response.
 */

#include "dshot.h"
#include "stm32f4xx.h"

/* DMA buffer for DShot frame transmission */
static uint16_t dshot_dma_buffer[DSHOT_FRAME_SIZE + 1];  /* +1 for trailing zero */

/* Input capture buffer for telemetry reception */
static uint16_t dshot_ic_buffer[DSHOT_IC_BUFFER_SIZE];
static volatile uint8_t ic_edge_count = 0;

/* State tracking */
static volatile dshot_state_t dshot_state = DSHOT_STATE_IDLE;

/* Telemetry data */
static dshot_telemetry_t telemetry = {0};
static volatile bool new_telemetry_available = false;

/* Simple tick counter for timing */
static volatile uint32_t tick_counter = 0;
static volatile uint32_t telem_start_time = 0;

/* GCR decoding lookup table
 * Maps 5-bit GCR symbols to 4-bit nibbles
 * Invalid codes map to 0xFF
 */
static const uint8_t gcr_decode_table[32] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  /* 0x00-0x07 */
    0xFF, 0x09, 0x0A, 0x0B, 0xFF, 0x0D, 0x0E, 0x0F,  /* 0x08-0x0F */
    0xFF, 0xFF, 0x02, 0x03, 0xFF, 0x05, 0x06, 0x07,  /* 0x10-0x17 */
    0xFF, 0x00, 0x08, 0x01, 0xFF, 0x04, 0x0C, 0xFF,  /* 0x18-0x1F */
};

/* Private function prototypes */
static uint16_t dshot_create_packet(uint16_t value, bool request_telemetry);
static void dshot_encode_dma_buffer(uint16_t packet);
static void dshot_switch_to_output(void);
static void dshot_switch_to_input(void);
static void dshot_start_input_capture(void);
static void dshot_stop_input_capture(void);
static bool dshot_decode_telemetry(void);
static uint32_t dshot_decode_gcr(uint32_t gcr_value);

/**
 * @brief Initialize bidirectional DShot protocol
 */
bool dshot_init(void) {
    /* Enable clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   /* Enable GPIOA clock */
    RCC->APB2ENR |= DSHOT_TIMER_RCC;        /* Enable timer clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;     /* Enable DMA2 clock */

    /* Configure GPIO for PWM output initially */
    dshot_switch_to_output();

    /* Configure Timer for DShot PWM */
    DSHOT_TIMER->CR1 = 0;                          /* Disable timer */
    DSHOT_TIMER->PSC = 0;                          /* No prescaler */
    DSHOT_TIMER->ARR = DSHOT_TIMER_PERIOD - 1;     /* Auto-reload value */

    /* Configure channel 1 for PWM output */
    DSHOT_TIMER->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);
    DSHOT_TIMER->CCMR1 |= (6 << 4);                /* PWM mode 1 */
    DSHOT_TIMER->CCMR1 |= TIM_CCMR1_OC1PE;         /* Preload enable */

    DSHOT_TIMER->CCER &= ~TIM_CCER_CC1P;           /* Active high */
    DSHOT_TIMER->CCER |= TIM_CCER_CC1E;            /* Enable output */

    DSHOT_TIMER->BDTR |= TIM_BDTR_MOE;             /* Main output enable */
    DSHOT_TIMER->DIER |= TIM_DIER_CC1DE;           /* Enable DMA requests on CC1 */

    DSHOT_TIMER->CCR1 = 0;                         /* Start with output low */

    /* Configure DMA Stream 1 for frame transmission (Memory to Peripheral) */
    DSHOT_DMA_STREAM->CR = 0;
    while (DSHOT_DMA_STREAM->CR & DMA_SxCR_EN);

    DSHOT_DMA_STREAM->CR = (DSHOT_DMA_CHANNEL << 25) |  /* Channel selection */
                           (2 << 16) |  /* Memory data size: 32-bit */
                           (2 << 13) |  /* Peripheral data size: 32-bit */
                           (1 << 10) |  /* Memory increment mode */
                           (1 << 6) |   /* Direction: Memory to peripheral */
                           (1 << 4);    /* Transfer complete interrupt enable */

    DSHOT_DMA_STREAM->PAR = (uint32_t)&DSHOT_TIMER->CCR1;
    DSHOT_DMA_STREAM->M0AR = (uint32_t)dshot_dma_buffer;
    DSHOT_DMA_STREAM->NDTR = DSHOT_FRAME_SIZE + 1;  /* +1 for trailing zero */

    /* Configure DMA Stream 6 for input capture (Peripheral to Memory) */
    DSHOT_IC_DMA_STREAM->CR = 0;
    while (DSHOT_IC_DMA_STREAM->CR & DMA_SxCR_EN);

    DSHOT_IC_DMA_STREAM->CR = (DSHOT_IC_DMA_CHANNEL << 25) |  /* Channel selection */
                              (1 << 16) |  /* Memory data size: 16-bit */
                              (1 << 13) |  /* Peripheral data size: 16-bit */
                              (1 << 10) |  /* Memory increment mode */
                              (0 << 6) |   /* Direction: Peripheral to memory */
                              (1 << 4);    /* Transfer complete interrupt enable */

    DSHOT_IC_DMA_STREAM->PAR = (uint32_t)&DSHOT_TIMER->CCR1;
    DSHOT_IC_DMA_STREAM->M0AR = (uint32_t)dshot_ic_buffer;
    DSHOT_IC_DMA_STREAM->NDTR = DSHOT_IC_BUFFER_SIZE;

    /* Initialize buffer with trailing zero to ensure clean signal end */
    dshot_dma_buffer[DSHOT_FRAME_SIZE] = 0;

    /* Enable DMA interrupts */
    NVIC_SetPriority(DMA2_Stream1_IRQn, 1);
    NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    NVIC_SetPriority(DMA2_Stream6_IRQn, 1);
    NVIC_EnableIRQ(DMA2_Stream6_IRQn);

    /* Enable timer */
    DSHOT_TIMER->CR1 |= TIM_CR1_CEN;

    dshot_state = DSHOT_STATE_IDLE;

    /* Initialize telemetry structure */
    telemetry.valid = false;
    telemetry.frame_count = 0;
    telemetry.success_count = 0;
    telemetry.error_count = 0;

    return true;
}

/**
 * @brief Switch GPIO to output mode (PWM)
 */
static void dshot_switch_to_output(void) {
    /* Disable input capture */
    DSHOT_TIMER->CCER &= ~TIM_CCER_CC1E;

    /* Configure for output compare (PWM) */
    DSHOT_TIMER->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);
    DSHOT_TIMER->CCMR1 |= (6 << 4) | TIM_CCMR1_OC1PE;  /* PWM mode 1, preload */

    /* GPIO: Alternate function mode for timer output */
    GPIOA->MODER &= ~(3 << (DSHOT_GPIO_PIN * 2));
    GPIOA->MODER |= (2 << (DSHOT_GPIO_PIN * 2));       /* AF mode */
    GPIOA->OSPEEDR |= (3 << (DSHOT_GPIO_PIN * 2));     /* Very high speed */
    GPIOA->PUPDR &= ~(3 << (DSHOT_GPIO_PIN * 2));      /* No pull */
    GPIOA->OTYPER &= ~(1 << DSHOT_GPIO_PIN);           /* Push-pull */

    /* Set alternate function */
    if (DSHOT_GPIO_PIN < 8) {
        GPIOA->AFR[0] &= ~(0xF << (DSHOT_GPIO_PIN * 4));
        GPIOA->AFR[0] |= (DSHOT_GPIO_AF << (DSHOT_GPIO_PIN * 4));
    } else {
        GPIOA->AFR[1] &= ~(0xF << ((DSHOT_GPIO_PIN - 8) * 4));
        GPIOA->AFR[1] |= (DSHOT_GPIO_AF << ((DSHOT_GPIO_PIN - 8) * 4));
    }

    /* Re-enable output compare */
    DSHOT_TIMER->CCER &= ~TIM_CCER_CC1P;              /* Active high */
    DSHOT_TIMER->CCER |= TIM_CCER_CC1E;               /* Enable output */

    /* Enable DMA requests for output */
    DSHOT_TIMER->DIER |= TIM_DIER_CC1DE;
}

/**
 * @brief Switch GPIO to input mode for telemetry capture
 */
static void dshot_switch_to_input(void) {
    /* Disable output */
    DSHOT_TIMER->CCER &= ~TIM_CCER_CC1E;
    DSHOT_TIMER->DIER &= ~TIM_DIER_CC1DE;

    /* Configure timer for input capture on channel 1 */
    DSHOT_TIMER->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M | TIM_CCMR1_OC1PE);
    DSHOT_TIMER->CCMR1 |= (1 << 0);  /* CC1S = 01: IC1 mapped to TI1 */

    /* GPIO: Still alternate function but timer will capture input */
    GPIOA->MODER &= ~(3 << (DSHOT_GPIO_PIN * 2));
    GPIOA->MODER |= (2 << (DSHOT_GPIO_PIN * 2));      /* AF mode */
    GPIOA->PUPDR &= ~(3 << (DSHOT_GPIO_PIN * 2));
    GPIOA->PUPDR |= (1 << (DSHOT_GPIO_PIN * 2));      /* Pull-up for idle high */

    /* Enable input capture on both edges */
    DSHOT_TIMER->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC1NP;
}

/**
 * @brief Start input capture DMA for telemetry
 */
static void dshot_start_input_capture(void) {
    /* Clear any pending flags */
    DMA2->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6 | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;
    DSHOT_TIMER->SR = 0;

    /* Reset buffer index */
    ic_edge_count = 0;

    /* Configure DMA for input capture */
    DSHOT_IC_DMA_STREAM->CR &= ~DMA_SxCR_EN;
    while (DSHOT_IC_DMA_STREAM->CR & DMA_SxCR_EN);

    DSHOT_IC_DMA_STREAM->NDTR = DSHOT_IC_BUFFER_SIZE;
    DSHOT_IC_DMA_STREAM->CR |= DMA_SxCR_EN;

    /* Enable DMA requests for input capture */
    DSHOT_TIMER->DIER |= TIM_DIER_CC1DE;

    telem_start_time = tick_counter;
}

/**
 * @brief Stop input capture
 */
static void dshot_stop_input_capture(void) {
    /* Disable DMA */
    DSHOT_TIMER->DIER &= ~TIM_DIER_CC1DE;
    DSHOT_IC_DMA_STREAM->CR &= ~DMA_SxCR_EN;

    /* Calculate how many edges we captured */
    ic_edge_count = DSHOT_IC_BUFFER_SIZE - DSHOT_IC_DMA_STREAM->NDTR;
}

/**
 * @brief Send throttle command to ESC
 */
void dshot_send_throttle(uint16_t throttle) {
    if (dshot_state != DSHOT_STATE_IDLE) {
        return;  /* Busy */
    }

    /* Clamp throttle value */
    if (throttle > DSHOT_THROTTLE_MAX) {
        throttle = DSHOT_THROTTLE_MAX;
    }

    /* Create packet with telemetry request bit SET for bidirectional DShot */
    uint16_t packet = dshot_create_packet(throttle, true);
    dshot_encode_dma_buffer(packet);

    /* Ensure we're in output mode */
    dshot_switch_to_output();

    /* Start DMA transfer */
    dshot_state = DSHOT_STATE_SENDING;
    telemetry.frame_count++;

    /* Clear DMA flags and start */
    DMA2->LIFCR = DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;

    DSHOT_DMA_STREAM->CR &= ~DMA_SxCR_EN;
    while (DSHOT_DMA_STREAM->CR & DMA_SxCR_EN);

    DSHOT_DMA_STREAM->NDTR = DSHOT_FRAME_SIZE + 1;
    DSHOT_DMA_STREAM->CR |= DMA_SxCR_EN;
}

/**
 * @brief Send special DShot command
 */
void dshot_send_command(uint8_t command) {
    if (command <= DSHOT_CMD_MAX) {
        /* Commands don't request telemetry */
        if (dshot_state != DSHOT_STATE_IDLE) {
            return;
        }

        uint16_t packet = dshot_create_packet(command, false);
        dshot_encode_dma_buffer(packet);

        dshot_switch_to_output();
        dshot_state = DSHOT_STATE_SENDING;

        DMA2->LIFCR = DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;

        DSHOT_DMA_STREAM->CR &= ~DMA_SxCR_EN;
        while (DSHOT_DMA_STREAM->CR & DMA_SxCR_EN);

        DSHOT_DMA_STREAM->NDTR = DSHOT_FRAME_SIZE + 1;
        DSHOT_DMA_STREAM->CR |= DMA_SxCR_EN;
    }
}

/**
 * @brief Check if DShot is ready to send
 */
bool dshot_ready(void) {
    return dshot_state == DSHOT_STATE_IDLE;
}

/**
 * @brief Get current state
 */
dshot_state_t dshot_get_state(void) {
    return dshot_state;
}

/**
 * @brief Get telemetry data
 */
dshot_telemetry_t* dshot_get_telemetry(void) {
    return &telemetry;
}

/**
 * @brief Check if new telemetry available
 */
bool dshot_telemetry_available(void) {
    bool available = new_telemetry_available;
    new_telemetry_available = false;
    return available;
}

/**
 * @brief Create DShot packet with CRC
 */
static uint16_t dshot_create_packet(uint16_t value, bool request_telemetry) {
    /* Packet format: [11-bit value][1-bit telemetry][4-bit CRC] */
    uint16_t packet = (value << 1) | (request_telemetry ? 1 : 0);

    /* Calculate 4-bit CRC (XOR of nibbles) */
    uint16_t crc = (packet ^ (packet >> 4) ^ (packet >> 8)) & 0x0F;

    return (packet << 4) | crc;
}

/**
 * @brief Encode DShot packet into DMA buffer
 *
 * For bidirectional DShot, the signal is INVERTED:
 * - Idle state is HIGH
 * - '0' bit: 62.5% high, 37.5% low
 * - '1' bit: 25% high, 75% low
 *
 * Using inverted PWM mode to achieve this.
 */
static void dshot_encode_dma_buffer(uint16_t packet) {
    /* For inverted DShot (bidirectional), we invert the duty cycles */
    uint16_t bit_0_duty = DSHOT_TIMER_PERIOD - DSHOT_BIT_0_DUTY;  /* ~62.5% for '0' */
    uint16_t bit_1_duty = DSHOT_TIMER_PERIOD - DSHOT_BIT_1_DUTY;  /* ~25% for '1' */

    for (int i = 0; i < DSHOT_FRAME_SIZE; i++) {
        if (packet & 0x8000) {
            dshot_dma_buffer[i] = bit_1_duty;
        } else {
            dshot_dma_buffer[i] = bit_0_duty;
        }
        packet <<= 1;
    }
    /* Trailing zero to end the frame cleanly */
    dshot_dma_buffer[DSHOT_FRAME_SIZE] = DSHOT_TIMER_PERIOD;  /* Full high for idle */
}

/**
 * @brief Process bidirectional telemetry state machine
 */
void dshot_update(void) {
    tick_counter++;

    switch (dshot_state) {
        case DSHOT_STATE_WAIT_TELEM:
            /* Wait for response window, then start capture */
            if ((tick_counter - telem_start_time) >= 1) {  /* ~1ms tick, adjust as needed */
                dshot_switch_to_input();
                dshot_start_input_capture();
                dshot_state = DSHOT_STATE_RECEIVING;
            }
            break;

        case DSHOT_STATE_RECEIVING:
            /* Check if we've captured enough edges or timed out */
            {
                uint8_t captured = DSHOT_IC_BUFFER_SIZE - DSHOT_IC_DMA_STREAM->NDTR;
                if (captured >= 20 || (tick_counter - telem_start_time) >= 2) {
                    dshot_stop_input_capture();
                    dshot_state = DSHOT_STATE_PROCESSING;
                }
            }
            break;

        case DSHOT_STATE_PROCESSING:
            /* Decode the captured telemetry */
            if (dshot_decode_telemetry()) {
                telemetry.valid = true;
                telemetry.success_count++;
                telemetry.last_update = tick_counter;
                new_telemetry_available = true;
            } else {
                telemetry.error_count++;
            }
            dshot_switch_to_output();
            dshot_state = DSHOT_STATE_IDLE;
            break;

        default:
            break;
    }
}

/**
 * @brief Decode GCR value to nibbles
 *
 * GCR encoding uses 5 bits to represent 4 bits, ensuring no long runs
 * of same bits which helps with signal integrity.
 */
static uint32_t dshot_decode_gcr(uint32_t gcr_value) {
    uint32_t result = 0;

    /* Extract 4 nibbles from 20 GCR bits (MSB first) */
    for (int i = 0; i < 4; i++) {
        uint8_t gcr_symbol = (gcr_value >> (15 - i * 5)) & 0x1F;
        uint8_t nibble = gcr_decode_table[gcr_symbol];

        if (nibble == 0xFF) {
            return 0xFFFFFFFF;  /* Invalid GCR symbol */
        }

        result = (result << 4) | nibble;
    }

    return result;
}

/**
 * @brief Decode telemetry from captured edges
 *
 * The ESC sends a 21-bit GCR-encoded response:
 * - 20 bits of GCR data (4 nibbles × 5 bits each)
 * - 1 stop/marker bit
 *
 * The decoded 16 bits contain:
 * - Bits 15-4: 12-bit eRPM period in microseconds
 * - Bits 3-0: 4-bit CRC
 */
static bool dshot_decode_telemetry(void) {
    if (ic_edge_count < 2) {
        return false;  /* Not enough edges */
    }

    /* Calculate bit period from captured edges
     * The response is at 5/4 the command rate (750kbps for DShot600)
     * Bit time = 168MHz / 750000 = 224 timer ticks
     */
    uint16_t bit_period = (TIMER_CLOCK_HZ / DSHOT_TELEM_BITRATE);
    uint16_t half_bit = bit_period / 2;

    /* Decode edges to bits using timing analysis */
    uint32_t gcr_bits = 0;
    uint8_t bit_count = 0;
    uint8_t current_level = 1;  /* Start high (idle) */

    for (int i = 1; i < ic_edge_count && bit_count < 21; i++) {
        /* Calculate time between edges */
        uint16_t delta;
        if (dshot_ic_buffer[i] >= dshot_ic_buffer[i-1]) {
            delta = dshot_ic_buffer[i] - dshot_ic_buffer[i-1];
        } else {
            /* Timer wrapped */
            delta = (0xFFFF - dshot_ic_buffer[i-1]) + dshot_ic_buffer[i] + 1;
        }

        /* Determine how many bits this edge represents */
        uint8_t num_bits = (delta + half_bit) / bit_period;
        if (num_bits == 0) num_bits = 1;
        if (num_bits > 5) num_bits = 5;  /* Limit to reasonable value */

        /* Add bits to result */
        for (int b = 0; b < num_bits && bit_count < 21; b++) {
            gcr_bits = (gcr_bits << 1) | current_level;
            bit_count++;
        }

        /* Toggle level after edge */
        current_level ^= 1;
    }

    if (bit_count < 20) {
        return false;  /* Not enough bits decoded */
    }

    /* Extract the 20 GCR bits (ignore the 21st marker bit) */
    uint32_t gcr_value = (gcr_bits >> 1) & 0xFFFFF;

    /* Decode GCR to get 16-bit value */
    uint32_t decoded = dshot_decode_gcr(gcr_value);
    if (decoded == 0xFFFFFFFF) {
        return false;  /* GCR decode error */
    }

    /* Extract eRPM period (bits 15-4) and CRC (bits 3-0) */
    uint16_t period = (decoded >> 4) & 0x0FFF;
    uint8_t received_crc = decoded & 0x0F;

    /* Verify CRC */
    uint8_t calculated_crc = (~(decoded >> 4)) & 0x0F;
    calculated_crc = (calculated_crc ^ (calculated_crc >> 4)) & 0x0F;

    /* Alternative CRC: XOR of nibbles */
    uint16_t temp = decoded >> 4;
    uint8_t crc_check = (temp ^ (temp >> 4) ^ (temp >> 8)) & 0x0F;

    if (received_crc != crc_check) {
        return false;  /* CRC mismatch */
    }

    /* Store telemetry data */
    telemetry.period_us = period;

    /* Calculate eRPM from period
     * period is in 1/16th microsecond units for EDT (extended telemetry)
     * or microseconds for basic telemetry
     *
     * eRPM = 60,000,000 / period_us (for period in microseconds)
     * Actual RPM = eRPM * 2 / motor_poles
     */
    if (period > 0) {
        telemetry.erpm = 60000000UL / period;
        telemetry.rpm = (telemetry.erpm * 2) / MOTOR_POLES;
    } else {
        telemetry.erpm = 0;
        telemetry.rpm = 0;
    }

    return true;
}

/**
 * @brief DMA transfer complete interrupt handler (TX)
 */
void DMA2_Stream1_IRQHandler(void) {
    /* Clear interrupt flag */
    DMA2->LIFCR = DMA_LIFCR_CTCIF1;

    if (dshot_state == DSHOT_STATE_SENDING) {
        /* Frame sent - wait briefly then start telemetry capture */
        dshot_state = DSHOT_STATE_WAIT_TELEM;
        telem_start_time = tick_counter;
    }
}

/**
 * @brief DMA transfer complete interrupt handler (RX/Input Capture)
 */
void DMA2_Stream6_IRQHandler(void) {
    /* Clear interrupt flag */
    DMA2->HIFCR = DMA_HIFCR_CTCIF6;

    /* Buffer full - can process telemetry */
    if (dshot_state == DSHOT_STATE_RECEIVING) {
        dshot_stop_input_capture();
        dshot_state = DSHOT_STATE_PROCESSING;
    }
}
