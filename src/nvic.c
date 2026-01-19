/**
 * @file nvic.c
 * @brief NVIC (Nested Vectored Interrupt Controller) implementation
 */

#include "stm32f4xx.h"

#define NVIC_BASE             (0xE000E100UL)
#define NVIC                  ((NVIC_Type *)NVIC_BASE)

typedef struct {
    volatile uint32_t ISER[8];
    uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];
    uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];
    uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];
    uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];
    uint32_t RESERVED4[56];
    volatile uint8_t  IP[240];
    uint32_t RESERVED5[644];
    volatile uint32_t STIR;
} NVIC_Type;

/**
 * @brief Enable interrupt
 */
void NVIC_EnableIRQ(IRQn_Type IRQn) {
    NVIC->ISER[(((uint32_t)IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
}

/**
 * @brief Disable interrupt
 */
void NVIC_DisableIRQ(IRQn_Type IRQn) {
    NVIC->ICER[(((uint32_t)IRQn) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)IRQn) & 0x1FUL));
}

/**
 * @brief Set interrupt priority
 */
void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority) {
    NVIC->IP[((uint32_t)IRQn)] = (uint8_t)((priority << (8U - __NVIC_PRIO_BITS)) & 0xFFUL);
}
