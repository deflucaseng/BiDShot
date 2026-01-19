/**
 * @file dshot.c
 * @brief DShot protocol implementation (transmission only)
 *
 * Telemetry is handled separately via ESC serial telemetry on UART.
 */

#include "dshot.h"
#include "stm32f4xx.h"

/* DMA buffer for DShot frame transmission */
static uint16_t dshot_dma_buffer[DSHOT_FRAME_SIZE + 1];  /* +1 for trailing zero */

/* State tracking */
static volatile dshot_state_t dshot_state = DSHOT_STATE_IDLE;

/* Private function prototypes */
static uint16_t dshot_create_packet(uint16_t value);
static void dshot_encode_dma_buffer(uint16_t packet);

/**
 * @brief Initialize DShot protocol
 */
bool dshot_init(void) {
    /* Enable clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  /* Enable GPIOA clock */
    RCC->APB2ENR |= DSHOT_TIMER_RCC;       /* Enable timer clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;    /* Enable DMA2 clock */

    /* Configure GPIO for PWM output */
    GPIOA->MODER &= ~(3 << (DSHOT_GPIO_PIN * 2));
    GPIOA->MODER |= (2 << (DSHOT_GPIO_PIN * 2));  /* Alternate function mode */
    GPIOA->OSPEEDR |= (3 << (DSHOT_GPIO_PIN * 2)); /* Very high speed */

    /* Set alternate function */
    if (DSHOT_GPIO_PIN < 8) {
        GPIOA->AFR[0] &= ~(0xF << (DSHOT_GPIO_PIN * 4));
        GPIOA->AFR[0] |= (DSHOT_GPIO_AF << (DSHOT_GPIO_PIN * 4));
    } else {
        GPIOA->AFR[1] &= ~(0xF << ((DSHOT_GPIO_PIN - 8) * 4));
        GPIOA->AFR[1] |= (DSHOT_GPIO_AF << ((DSHOT_GPIO_PIN - 8) * 4));
    }

    /* Configure Timer for DShot PWM */
    DSHOT_TIMER->PSC = 0;                          /* No prescaler */
    DSHOT_TIMER->ARR = DSHOT_TIMER_PERIOD - 1;     /* Auto-reload value */
    DSHOT_TIMER->CCMR1 &= ~TIM_CCMR1_OC1M;
    DSHOT_TIMER->CCMR1 |= (6 << 4);                /* PWM mode 1 */
    DSHOT_TIMER->CCMR1 |= TIM_CCMR1_OC1PE;         /* Preload enable */
    DSHOT_TIMER->CCER |= TIM_CCER_CC1E;            /* Enable output */
    DSHOT_TIMER->BDTR |= TIM_BDTR_MOE;             /* Main output enable */
    DSHOT_TIMER->DIER |= TIM_DIER_CC1DE;           /* Enable DMA requests */

    /* Configure DMA for frame transmission */
    DSHOT_DMA_STREAM->CR = 0;
    while (DSHOT_DMA_STREAM->CR & DMA_SxCR_EN);

    DSHOT_DMA_STREAM->CR = (DSHOT_DMA_CHANNEL << 25) |  /* Channel selection */
                           (1 << 16) |  /* Memory data size: 16-bit */
                           (1 << 13) |  /* Peripheral data size: 16-bit */
                           (1 << 10) |  /* Memory increment mode */
                           (1 << 6) |   /* Direction: Memory to peripheral */
                           (1 << 4);    /* Transfer complete interrupt enable */

    DSHOT_DMA_STREAM->PAR = (uint32_t)&DSHOT_TIMER->CCR1;
    DSHOT_DMA_STREAM->M0AR = (uint32_t)dshot_dma_buffer;
    DSHOT_DMA_STREAM->NDTR = DSHOT_FRAME_SIZE + 1;  /* +1 for trailing zero */

    /* Initialize buffer with trailing zero to ensure clean signal end */
    dshot_dma_buffer[DSHOT_FRAME_SIZE] = 0;

    /* Enable DMA interrupt */
    NVIC_SetPriority(DMA2_Stream1_IRQn, 1);
    NVIC_EnableIRQ(DMA2_Stream1_IRQn);

    /* Enable timer */
    DSHOT_TIMER->CR1 |= TIM_CR1_CEN;

    dshot_state = DSHOT_STATE_IDLE;
    return true;
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

    uint16_t packet = dshot_create_packet(throttle);
    dshot_encode_dma_buffer(packet);

    /* Start DMA transfer */
    dshot_state = DSHOT_STATE_SENDING;

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
        dshot_send_throttle(command);
    }
}

/**
 * @brief Check if DShot is ready to send
 */
bool dshot_ready(void) {
    return dshot_state == DSHOT_STATE_IDLE;
}

/**
 * @brief Create DShot packet with CRC
 */
static uint16_t dshot_create_packet(uint16_t value) {
    /* Telemetry bit is always 0 - we use serial telemetry instead */
    uint16_t packet = (value << 1) | 0;

    /* Calculate 4-bit CRC */
    uint16_t crc = (packet ^ (packet >> 4) ^ (packet >> 8)) & 0x0F;

    return (packet << 4) | crc;
}

/**
 * @brief Encode DShot packet into DMA buffer
 */
static void dshot_encode_dma_buffer(uint16_t packet) {
    for (int i = 0; i < DSHOT_FRAME_SIZE; i++) {
        if (packet & 0x8000) {
            dshot_dma_buffer[i] = DSHOT_BIT_1_DUTY;
        } else {
            dshot_dma_buffer[i] = DSHOT_BIT_0_DUTY;
        }
        packet <<= 1;
    }
}

/**
 * @brief DMA transfer complete interrupt handler
 */
void DMA2_Stream1_IRQHandler(void) {
    /* Clear interrupt flag */
    DMA2->LIFCR |= DMA_LIFCR_CTCIF1;

    /* Frame sent, ready for next */
    dshot_state = DSHOT_STATE_IDLE;
}
