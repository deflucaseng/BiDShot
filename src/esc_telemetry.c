/**
 * @file esc_telemetry.c
 * @brief ESC Serial Telemetry reception implementation
 */

#include "esc_telemetry.h"
#include "stm32f4xx.h"

/* System clock configuration */
#define APB2_CLOCK_HZ           84000000UL  /* APB2 clock at 84MHz */

/* Receive buffer */
static uint8_t rx_buffer[ESC_TELEM_PACKET_SIZE];
static volatile uint8_t rx_index = 0;

/* Telemetry data */
static esc_telemetry_t telemetry_data = {0};
static volatile bool new_data_available = false;
static volatile uint32_t last_byte_time = 0;

/* Simple tick counter (incremented in main loop or systick) */
static volatile uint32_t tick_counter = 0;

/**
 * @brief Calculate CRC8 for telemetry packet
 */
static uint8_t esc_telemetry_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0xD5;  /* Polynomial used by KISS/BLHeli */
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief Parse received packet into telemetry structure
 */
static bool esc_telemetry_parse_packet(void) {
    /* Verify CRC */
    uint8_t calculated_crc = esc_telemetry_crc8(rx_buffer, ESC_TELEM_PACKET_SIZE - 1);
    if (calculated_crc != rx_buffer[9]) {
        return false;
    }

    /* Parse packet (big-endian format) */
    telemetry_data.temperature = rx_buffer[0];
    telemetry_data.voltage     = (rx_buffer[1] << 8) | rx_buffer[2];
    telemetry_data.current     = (rx_buffer[3] << 8) | rx_buffer[4];
    telemetry_data.consumption = (rx_buffer[5] << 8) | rx_buffer[6];
    telemetry_data.erpm        = (rx_buffer[7] << 8) | rx_buffer[8];

    /* Calculate actual RPM from eRPM */
    /* eRPM is electrical RPM / 100, actual RPM = (eRPM * 100) / (poles / 2) */
    telemetry_data.rpm = ((uint32_t)telemetry_data.erpm * 100 * 2) / MOTOR_POLES;

    telemetry_data.valid = true;
    telemetry_data.last_update = tick_counter;

    return true;
}

/**
 * @brief Initialize ESC telemetry reception
 */
bool esc_telemetry_init(void) {
    /* Enable clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;      /* Enable GPIOA clock */
    RCC->APB2ENR |= ESC_TELEM_USART_RCC;      /* Enable USART1 clock */

    /* Configure GPIO for USART1 RX (PA10) */
    /* Set to alternate function mode */
    GPIOA->MODER &= ~(3 << (ESC_TELEM_RX_PIN * 2));
    GPIOA->MODER |= (2 << (ESC_TELEM_RX_PIN * 2));

    /* Set to high speed */
    GPIOA->OSPEEDR |= (3 << (ESC_TELEM_RX_PIN * 2));

    /* Enable pull-up (helps with noise when ESC not connected) */
    GPIOA->PUPDR &= ~(3 << (ESC_TELEM_RX_PIN * 2));
    GPIOA->PUPDR |= (1 << (ESC_TELEM_RX_PIN * 2));

    /* Set alternate function AF7 for USART1 */
    GPIOA->AFR[1] &= ~(0xF << ((ESC_TELEM_RX_PIN - 8) * 4));
    GPIOA->AFR[1] |= (ESC_TELEM_GPIO_AF << ((ESC_TELEM_RX_PIN - 8) * 4));

    /* Configure USART1 */
    /* Calculate baud rate: BRR = fck / baud */
    uint32_t usartdiv = APB2_CLOCK_HZ / ESC_TELEM_BAUDRATE;
    ESC_TELEM_USART->BRR = usartdiv;

    /* Enable USART and receiver only (no transmit needed) */
    ESC_TELEM_USART->CR1 = USART_CR1_UE | USART_CR1_RE;

    /* Reset state */
    rx_index = 0;
    telemetry_data.valid = false;

    return true;
}

/**
 * @brief Increment tick counter (call from main loop or systick)
 */
void esc_telemetry_tick(void) {
    tick_counter++;
}

/**
 * @brief Process incoming telemetry data
 */
void esc_telemetry_update(void) {
    /* Check for timeout - reset buffer if too long between bytes */
    if (rx_index > 0 && (tick_counter - last_byte_time) > ESC_TELEM_TIMEOUT_MS) {
        rx_index = 0;  /* Reset on timeout */
    }

    /* Process all available bytes */
    while (ESC_TELEM_USART->SR & USART_SR_RXNE) {
        uint8_t byte = ESC_TELEM_USART->DR;
        last_byte_time = tick_counter;

        /* Store byte in buffer */
        if (rx_index < ESC_TELEM_PACKET_SIZE) {
            rx_buffer[rx_index++] = byte;
        }

        /* Check if we have a complete packet */
        if (rx_index >= ESC_TELEM_PACKET_SIZE) {
            if (esc_telemetry_parse_packet()) {
                new_data_available = true;
            }
            rx_index = 0;  /* Reset for next packet */
        }
    }

    /* Handle overrun error */
    if (ESC_TELEM_USART->SR & USART_SR_ORE) {
        (void)ESC_TELEM_USART->DR;  /* Clear by reading DR */
        rx_index = 0;
    }
}

/**
 * @brief Get the latest telemetry data
 */
esc_telemetry_t* esc_telemetry_get(void) {
    return &telemetry_data;
}

/**
 * @brief Check if new telemetry data is available
 */
bool esc_telemetry_available(void) {
    bool available = new_data_available;
    new_data_available = false;
    return available;
}

/**
 * @brief Get voltage as float in volts
 */
float esc_telemetry_get_voltage_v(void) {
    return (float)telemetry_data.voltage / 100.0f;
}

/**
 * @brief Get current as float in amps
 */
float esc_telemetry_get_current_a(void) {
    return (float)telemetry_data.current / 100.0f;
}
