/**
 * @file stm32f4xx.h
 * @brief Minimal STM32F4 register definitions
 * 
 * For a complete project, use the official CMSIS headers from ST.
 * This is a simplified version for demonstration.
 */

#ifndef STM32F4XX_H
#define STM32F4XX_H

#include <stdint.h>

/* Core peripherals */
#define __CM4_REV                 0x0001U
#define __MPU_PRESENT             1U
#define __NVIC_PRIO_BITS          4U
#define __Vendor_SysTickConfig    0U
#define __FPU_PRESENT             1U

/* Interrupt Numbers */
typedef enum {
    DMA2_Stream1_IRQn = 57,
    TIM1_CC_IRQn = 27,
} IRQn_Type;

/* Memory Base Addresses */
#define FLASH_BASE            0x08000000UL
#define SRAM_BASE             0x20000000UL
#define PERIPH_BASE           0x40000000UL
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000UL)

/* Peripheral addresses */
#define GPIOA_BASE            (AHB1PERIPH_BASE + 0x0000UL)
#define RCC_BASE              (AHB1PERIPH_BASE + 0x3800UL)
#define FLASH_R_BASE          (AHB1PERIPH_BASE + 0x3C00UL)
#define DMA2_BASE             (AHB1PERIPH_BASE + 0x6400UL)
#define DMA2_Stream1_BASE     (DMA2_BASE + 0x0028UL)
#define USART2_BASE           (APB1PERIPH_BASE + 0x4400UL)
#define TIM1_BASE             (APB2PERIPH_BASE + 0x0000UL)

/* GPIO */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

/* RCC */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

/* TIM */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    volatile uint32_t RCR;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    volatile uint32_t BDTR;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} TIM_TypeDef;

/* DMA */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t NDTR;
    volatile uint32_t PAR;
    volatile uint32_t M0AR;
    volatile uint32_t M1AR;
    volatile uint32_t FCR;
} DMA_Stream_TypeDef;

typedef struct {
    volatile uint32_t LISR;
    volatile uint32_t HISR;
    volatile uint32_t LIFCR;
    volatile uint32_t HIFCR;
} DMA_TypeDef;

/* USART */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

/* FLASH */
typedef struct {
    volatile uint32_t ACR;
    volatile uint32_t KEYR;
    volatile uint32_t OPTKEYR;
    volatile uint32_t SR;
    volatile uint32_t CR;
    volatile uint32_t OPTCR;
} FLASH_TypeDef;

/* SCB (System Control Block) */
typedef struct {
    volatile uint32_t CPUID;
    volatile uint32_t ICSR;
    volatile uint32_t VTOR;
    volatile uint32_t AIRCR;
    volatile uint32_t SCR;
    volatile uint32_t CCR;
    volatile uint8_t  SHP[12];
    volatile uint32_t SHCSR;
    volatile uint32_t CFSR;
    volatile uint32_t HFSR;
    volatile uint32_t DFSR;
    volatile uint32_t MMFAR;
    volatile uint32_t BFAR;
    volatile uint32_t AFSR;
    volatile uint32_t CPACR;
} SCB_Type;

#define SCB_BASE              (0xE000ED00UL)
#define SCB                   ((SCB_Type *)SCB_BASE)

/* Peripheral pointers */
#define GPIOA                 ((GPIO_TypeDef *)GPIOA_BASE)
#define RCC                   ((RCC_TypeDef *)RCC_BASE)
#define TIM1                  ((TIM_TypeDef *)TIM1_BASE)
#define DMA2                  ((DMA_TypeDef *)DMA2_BASE)
#define DMA2_Stream1          ((DMA_Stream_TypeDef *)DMA2_Stream1_BASE)
#define USART2                ((USART_TypeDef *)USART2_BASE)
#define FLASH                 ((FLASH_TypeDef *)FLASH_R_BASE)

/* RCC bit definitions */
#define RCC_CR_HSION          (1UL << 0)
#define RCC_CR_HSIRDY         (1UL << 1)
#define RCC_CR_HSEON          (1UL << 16)
#define RCC_CR_HSERDY         (1UL << 17)
#define RCC_CR_PLLON          (1UL << 24)
#define RCC_CR_PLLRDY         (1UL << 25)
#define RCC_CFGR_SW_PLL       (2UL << 0)
#define RCC_CFGR_SWS          (3UL << 2)
#define RCC_CFGR_SWS_PLL      (2UL << 2)
#define RCC_CFGR_PPRE1_DIV4   (5UL << 10)
#define RCC_CFGR_PPRE2_DIV2   (4UL << 13)
#define RCC_PLLCFGR_PLLSRC    (1UL << 22)
#define RCC_PLLCFGR_PLLM      (0x3FUL << 0)
#define RCC_PLLCFGR_PLLN      (0x1FFUL << 6)
#define RCC_PLLCFGR_PLLP      (3UL << 16)
#define RCC_AHB1ENR_GPIOAEN   (1UL << 0)
#define RCC_AHB1ENR_DMA2EN    (1UL << 22)
#define RCC_APB1ENR_USART2EN  (1UL << 17)
#define RCC_APB2ENR_TIM1EN    (1UL << 0)

/* TIM bit definitions */
#define TIM_CR1_CEN           (1UL << 0)
#define TIM_DIER_CC1DE        (1UL << 9)
#define TIM_DIER_CC1IE        (1UL << 1)
#define TIM_SR_CC1IF          (1UL << 1)
#define TIM_CCMR1_OC1M        (7UL << 4)
#define TIM_CCMR1_OC1PE       (1UL << 3)
#define TIM_CCMR1_CC1S        (3UL << 0)
#define TIM_CCMR1_CC1S_0      (1UL << 0)
#define TIM_CCER_CC1E         (1UL << 0)
#define TIM_CCER_CC1P         (1UL << 1)
#define TIM_CCER_CC1NP        (1UL << 3)
#define TIM_BDTR_MOE          (1UL << 15)

/* DMA bit definitions */
#define DMA_SxCR_EN           (1UL << 0)
#define DMA_LIFCR_CTCIF1      (1UL << 11)

/* USART bit definitions */
#define USART_SR_TXE          (1UL << 7)
#define USART_SR_RXNE         (1UL << 5)
#define USART_CR1_UE          (1UL << 13)
#define USART_CR1_TE          (1UL << 3)
#define USART_CR1_RE          (1UL << 2)

/* FLASH bit definitions */
#define FLASH_ACR_LATENCY_5WS (5UL << 0)
#define FLASH_ACR_PRFTEN      (1UL << 8)
#define FLASH_ACR_ICEN        (1UL << 9)
#define FLASH_ACR_DCEN        (1UL << 10)

/* NVIC functions */
void NVIC_EnableIRQ(IRQn_Type IRQn);
void NVIC_DisableIRQ(IRQn_Type IRQn);
void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority);

/* System functions */
void SystemInit(void);
void SystemCoreClockUpdate(void);

/* NOP instruction */
#define __NOP() __asm volatile ("nop")

extern uint32_t SystemCoreClock;

#endif // STM32F4XX_H
