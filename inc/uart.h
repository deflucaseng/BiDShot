/**
 * @file uart.h
 * @brief UART driver for serial communication
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

/* UART Configuration */
#define UART_BAUDRATE           115200
#define UART_PORT               USART2      // USART2 is commonly on PA2/PA3
#define UART_GPIO_PORT          GPIOA
#define UART_TX_PIN             2           // PA2
#define UART_RX_PIN             3           // PA3
#define UART_GPIO_AF            7           // AF7 for USART2

/**
 * @brief Initialize UART
 * @param baudrate Desired baud rate
 * @return true if successful
 */
bool uart_init(uint32_t baudrate);

/**
 * @brief Send a single character
 * @param c Character to send
 */
void uart_putc(char c);

/**
 * @brief Send a string
 * @param str Null-terminated string
 */
void uart_puts(const char* str);

/**
 * @brief Send formatted string (basic printf-like)
 * @param format Format string
 * @param ... Variable arguments
 */
void uart_printf(const char* format, ...);

/**
 * @brief Check if data is available to read
 * @return true if data available
 */
bool uart_available(void);

/**
 * @brief Read a single character
 * @return Character read
 */
char uart_getc(void);

#endif // UART_H
