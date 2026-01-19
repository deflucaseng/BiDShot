/**
 * @file main.c
 * @brief Main application for DShot motor control with serial ESC telemetry
 *
 * This demonstrates:
 * - DShot600 motor control (transmission only)
 * - Serial ESC telemetry reception (KISS/BLHeli32 protocol)
 * - Real-time display of RPM, voltage, current, temperature
 */

#include "dshot.h"
#include "esc_telemetry.h"
#include "uart.h"
#include "stm32f4xx.h"
#include <stdbool.h>

/* Delay function (busy wait) */
static void delay_ms(uint32_t ms) {
    // Approximate delay at 168MHz
    for (uint32_t i = 0; i < ms * 21000; i++) {
        __NOP();
    }
}

/* Simple microsecond delay */
static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us * 21; i++) {
        __NOP();
    }
}

/**
 * @brief Initialize system clock to 168MHz (for STM32F4)
 */
void system_clock_init(void) {
    // Enable HSE (external oscillator, typically 8MHz)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));
    
    // Configure PLL: HSE * (336/8) / 2 = 168MHz
    RCC->PLLCFGR = (336 << 6) |  // PLLN = 336
                   (8 << 0) |     // PLLM = 8
                   (0 << 16) |    // PLLP = 2
                   (1 << 22);     // PLL source = HSE
    
    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));
    
    // Configure flash latency (5 wait states for 168MHz)
    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_5WS;
    
    // Set APB1 prescaler to /4 (42MHz) and APB2 to /2 (84MHz)
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;
    
    // Switch system clock to PLL
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

/**
 * @brief Arm the ESC with proper initialization sequence
 */
void esc_arm_sequence(void) {
    uart_puts("\r\n=== ESC Arming Sequence ===\r\n");

    /* Send zero throttle for a period */
    uart_puts("Sending zero throttle...\r\n");
    for (int i = 0; i < 100; i++) {
        dshot_send_throttle(DSHOT_THROTTLE_MIN);
        esc_telemetry_update();  /* Keep processing telemetry */
        delay_ms(10);
    }

    /* Optional: Send beep command to confirm ESC is responding */
    uart_puts("Sending beep command...\r\n");
    for (int i = 0; i < 10; i++) {
        dshot_send_command(DSHOT_CMD_BEEP1);
        esc_telemetry_update();
        delay_ms(10);
    }

    delay_ms(500);
    uart_puts("ESC armed and ready!\r\n\r\n");
}

/**
 * @brief Test motor at various speeds and display telemetry
 */
void motor_test_cycle(void) {
    uint16_t test_throttles[] = {
        DSHOT_THROTTLE_MIN,       /* Min (stopped) */
        DSHOT_THROTTLE_MIN + 100, /* Low speed */
        DSHOT_THROTTLE_MIN + 300, /* Medium-low */
        DSHOT_THROTTLE_MIN + 500, /* Medium */
        DSHOT_THROTTLE_MIN + 700, /* Medium-high */
        DSHOT_THROTTLE_MIN + 1000 /* Higher (be careful!) */
    };

    int num_steps = sizeof(test_throttles) / sizeof(test_throttles[0]);

    uart_puts("\r\n=== Starting Motor Test Cycle ===\r\n");
    uart_puts("WARNING: Remove propellers before testing!\r\n\r\n");

    for (int step = 0; step < num_steps; step++) {
        uint16_t throttle = test_throttles[step];

        uart_printf("Throttle: %u\r\n", throttle);

        /* Run at this throttle for ~1 second */
        for (int i = 0; i < 50; i++) {
            dshot_send_throttle(throttle);
            esc_telemetry_update();
            esc_telemetry_tick();

            /* Check for telemetry data */
            if (esc_telemetry_available()) {
                esc_telemetry_t* telem = esc_telemetry_get();
                uart_printf("  RPM: %u | Temp: %u C | V: %u.%02uV | I: %u.%02uA\r\n",
                           telem->rpm,
                           telem->temperature,
                           telem->voltage / 100, telem->voltage % 100,
                           telem->current / 100, telem->current % 100);
            }

            delay_ms(20);  /* 50Hz update rate */
        }

        delay_ms(500);
    }

    /* Ramp back down to zero */
    uart_puts("\r\nRamping down...\r\n");
    for (int throttle = DSHOT_THROTTLE_MIN + 500; throttle >= DSHOT_THROTTLE_MIN; throttle -= 50) {
        for (int i = 0; i < 10; i++) {
            dshot_send_throttle(throttle);
            esc_telemetry_update();
            delay_ms(10);
        }
    }

    uart_puts("Test cycle complete!\r\n\r\n");
}

/**
 * @brief Interactive control mode - read commands from serial
 */
void interactive_mode(void) {
    uint16_t current_throttle = DSHOT_THROTTLE_MIN;
    uint32_t display_counter = 0;

    uart_puts("\r\n=== Interactive Mode ===\r\n");
    uart_puts("Commands:\r\n");
    uart_puts("  +: Increase throttle by 50\r\n");
    uart_puts("  -: Decrease throttle by 50\r\n");
    uart_puts("  0: Stop motor\r\n");
    uart_puts("  b: Send beep command\r\n");
    uart_puts("  t: Run test cycle\r\n");
    uart_puts("  h: Show this help\r\n");
    uart_puts("\r\nReady for commands...\r\n\r\n");

    while (1) {
        /* Send current throttle command */
        dshot_send_throttle(current_throttle);

        /* Process ESC telemetry */
        esc_telemetry_update();
        esc_telemetry_tick();

        /* Display telemetry periodically (every ~500ms) */
        display_counter++;
        if (display_counter >= 25) {
            display_counter = 0;

            esc_telemetry_t* telem = esc_telemetry_get();
            if (telem->valid) {
                uart_printf("[Thr: %u | RPM: %u | %u C | %u.%02uV | %u.%02uA | %umAh]\r\n",
                           current_throttle,
                           telem->rpm,
                           telem->temperature,
                           telem->voltage / 100, telem->voltage % 100,
                           telem->current / 100, telem->current % 100,
                           telem->consumption);
            }
        }

        /* Check for user input */
        if (uart_available()) {
            char cmd = uart_getc();
            uart_putc(cmd);  /* Echo */
            uart_puts("\r\n");

            switch (cmd) {
                case '+':
                    if (current_throttle < DSHOT_THROTTLE_MAX - 50) {
                        current_throttle += 50;
                        uart_printf("Throttle increased to %u\r\n", current_throttle);
                    }
                    break;

                case '-':
                    if (current_throttle > DSHOT_THROTTLE_MIN + 50) {
                        current_throttle -= 50;
                        uart_printf("Throttle decreased to %u\r\n", current_throttle);
                    }
                    break;

                case '0':
                    current_throttle = DSHOT_THROTTLE_MIN;
                    uart_puts("Motor stopped\r\n");
                    break;

                case 'b':
                    uart_puts("Sending beep...\r\n");
                    for (int i = 0; i < 10; i++) {
                        dshot_send_command(DSHOT_CMD_BEEP1);
                        esc_telemetry_update();
                        delay_ms(10);
                    }
                    break;

                case 't':
                    motor_test_cycle();
                    current_throttle = DSHOT_THROTTLE_MIN;
                    break;

                case 'h':
                    uart_puts("Commands: +/- (throttle), 0 (stop), b (beep), t (test), h (help)\r\n");
                    break;

                default:
                    uart_puts("Unknown command. Press 'h' for help.\r\n");
                    break;
            }
        }

        delay_ms(20);  /* 50Hz update rate */
    }
}

/**
 * @brief Main function
 */
int main(void) {
    /* Initialize system */
    system_clock_init();

    /* Initialize UART for serial output */
    uart_init(UART_BAUDRATE);

    /* Startup message */
    uart_puts("\r\n\r\n");
    uart_puts("========================================\r\n");
    uart_puts("  DShot600 Motor Controller\r\n");
    uart_puts("  with Serial ESC Telemetry\r\n");
    uart_puts("========================================\r\n");
    uart_puts("\r\n");

    /* Initialize DShot */
    uart_puts("Initializing DShot...\r\n");
    if (!dshot_init()) {
        uart_puts("ERROR: DShot initialization failed!\r\n");
        while (1);
    }
    uart_puts("DShot initialized successfully.\r\n");

    /* Initialize ESC telemetry */
    uart_puts("Initializing ESC telemetry...\r\n");
    if (!esc_telemetry_init()) {
        uart_puts("ERROR: ESC telemetry initialization failed!\r\n");
        while (1);
    }
    uart_puts("ESC telemetry initialized (USART1 RX on PA10).\r\n");

    delay_ms(1000);

    /* Arm ESC */
    esc_arm_sequence();

    /* Choose mode */
    uart_puts("Select mode:\r\n");
    uart_puts("  1: Automatic test cycle\r\n");
    uart_puts("  2: Interactive mode\r\n");
    uart_puts("\r\nWaiting for selection...\r\n");

    char mode = '2';  /* Default to interactive mode */
    if (uart_available()) {
        mode = uart_getc();
    } else {
        /* Wait a bit for input, otherwise default to interactive */
        for (int i = 0; i < 300; i++) {
            if (uart_available()) {
                mode = uart_getc();
                break;
            }
            esc_telemetry_update();
            delay_ms(10);
        }
    }

    if (mode == '1') {
        uart_puts("\r\nStarting automatic test cycle...\r\n");
        while (1) {
            motor_test_cycle();
            delay_ms(5000);
        }
    } else {
        interactive_mode();
    }

    return 0;
}
