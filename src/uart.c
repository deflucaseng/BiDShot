/**
 * @file uart.c
 * @brief UART driver implementation
 */

#include "uart.h"
#include "stm32f4xx.h"
#include <stdarg.h>
#include <stdio.h>

/* System clock frequency (adjust based on your config) */
#define SYSCLK_HZ               168000000UL
#define APB1_PRESCALER          4
#define APB1_CLOCK_HZ           (SYSCLK_HZ / APB1_PRESCALER)

/**
 * @brief Initialize UART
 */
bool uart_init(uint32_t baudrate) {
    /* Enable clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;    // Enable GPIOA clock
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;   // Enable USART2 clock

    /* Configure GPIO pins for UART */
    // Set PA2 (TX) and PA3 (RX) to alternate function mode
    GPIOA->MODER &= ~((3 << (UART_TX_PIN * 2)) | (3 << (UART_RX_PIN * 2)));
    GPIOA->MODER |= (2 << (UART_TX_PIN * 2)) | (2 << (UART_RX_PIN * 2));
    
    // Set to high speed
    GPIOA->OSPEEDR |= (3 << (UART_TX_PIN * 2)) | (3 << (UART_RX_PIN * 2));
    
    // Set alternate function (AF7 for USART2)
    GPIOA->AFR[0] &= ~((0xF << (UART_TX_PIN * 4)) | (0xF << (UART_RX_PIN * 4)));
    GPIOA->AFR[0] |= (UART_GPIO_AF << (UART_TX_PIN * 4)) | (UART_GPIO_AF << (UART_RX_PIN * 4));

    /* Configure UART */
    // Calculate baud rate
    uint32_t usartdiv = APB1_CLOCK_HZ / baudrate;
    UART_PORT->BRR = usartdiv;
    
    // Enable UART, transmitter, and receiver
    UART_PORT->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;

    return true;
}

/**
 * @brief Send a single character
 */
void uart_putc(char c) {
    // Wait for transmit data register to be empty
    while (!(UART_PORT->SR & USART_SR_TXE));
    
    UART_PORT->DR = c;
}

/**
 * @brief Send a string
 */
void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

/**
 * @brief Simple integer to string conversion
 */
static void itoa_simple(int32_t value, char* buffer, int base) {
    char* ptr = buffer;
    char* ptr1 = buffer;
    char tmp_char;
    int32_t tmp_value;

    if (value < 0 && base == 10) {
        value = -value;
        *ptr++ = '-';
        ptr1++;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';
    
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

/**
 * @brief Send formatted string (simplified printf)
 */
void uart_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[32];
    const char* p = format;
    
    while (*p) {
        if (*p == '%') {
            p++;
            switch (*p) {
                case 'd':
                case 'i': {
                    int32_t val = va_arg(args, int32_t);
                    itoa_simple(val, buffer, 10);
                    uart_puts(buffer);
                    break;
                }
                case 'u': {
                    uint32_t val = va_arg(args, uint32_t);
                    itoa_simple(val, buffer, 10);
                    uart_puts(buffer);
                    break;
                }
                case 'x':
                case 'X': {
                    uint32_t val = va_arg(args, uint32_t);
                    itoa_simple(val, buffer, 16);
                    uart_puts(buffer);
                    break;
                }
                case 's': {
                    char* str = va_arg(args, char*);
                    uart_puts(str);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    uart_putc(c);
                    break;
                }
                case '%':
                    uart_putc('%');
                    break;
                default:
                    uart_putc('%');
                    uart_putc(*p);
                    break;
            }
            p++;
        } else {
            uart_putc(*p++);
        }
    }
    
    va_end(args);
}

/**
 * @brief Check if data is available to read
 */
bool uart_available(void) {
    return (UART_PORT->SR & USART_SR_RXNE) != 0;
}

/**
 * @brief Read a single character
 */
char uart_getc(void) {
    // Wait for data to be received
    while (!uart_available());
    
    return (char)UART_PORT->DR;
}
