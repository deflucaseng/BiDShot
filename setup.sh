#!/bin/bash

# Script to download necessary CMSIS and startup files for STM32F4
# This is optional - you can also download STM32CubeF4 manually

echo "========================================="
echo "DShot STM32 Project Setup Helper"
echo "========================================="
echo ""

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CMSIS_DIR="$PROJECT_DIR/CMSIS"
STARTUP_DIR="$PROJECT_DIR/startup"
LINKER_DIR="$PROJECT_DIR/linker"

# Create directories
mkdir -p "$CMSIS_DIR/Include"
mkdir -p "$CMSIS_DIR/Device/ST/STM32F4xx/Include"
mkdir -p "$STARTUP_DIR"
mkdir -p "$LINKER_DIR"

echo "Note: This script requires manual file download from ST."
echo ""
echo "Please visit: https://www.st.com/en/embedded-software/stm32cubef4.html"
echo ""
echo "After downloading and extracting STM32CubeF4, copy these files:"
echo ""
echo "1. CMSIS Core headers:"
echo "   From: STM32Cube_FW_F4_VX.XX.X/Drivers/CMSIS/Include/*"
echo "   To:   $CMSIS_DIR/Include/"
echo ""
echo "2. STM32F4 Device headers:"
echo "   From: STM32Cube_FW_F4_VX.XX.X/Drivers/CMSIS/Device/ST/STM32F4xx/Include/*"
echo "   To:   $CMSIS_DIR/Device/ST/STM32F4xx/Include/"
echo ""
echo "3. Startup file (for STM32F411):"
echo "   From: STM32Cube_FW_F4_VX.XX.X/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s"
echo "   To:   $STARTUP_DIR/"
echo ""
echo "4. Linker script:"
echo "   From: STM32Cube_FW_F4_VX.XX.X/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/linker/STM32F411xE.ld"
echo "   To:   $LINKER_DIR/"
echo ""
echo "Note: For other STM32F4 variants, use the appropriate startup and linker files."
echo ""

# Alternative: Minimal setup without full CMSIS
echo "========================================="
echo "Quick Start Option (No Download Needed)"
echo "========================================="
echo ""
echo "The project includes minimal headers in inc/stm32f4xx.h"
echo "This is sufficient for basic operation, but we recommend"
echo "using full CMSIS headers for production projects."
echo ""
echo "To use minimal headers:"
echo "1. Create minimal startup file"
echo "2. Create basic linker script"
echo ""

read -p "Create minimal startup and linker files? (y/n) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Creating minimal startup file..."
    
    cat > "$STARTUP_DIR/startup_stm32f411xe_minimal.s" << 'EOF'
.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.global g_pfnVectors
.global Default_Handler

/* start address for the initialization values of the .data section */
.word _sidata
/* start address for the .data section */
.word _sdata
/* end address for the .data section */
.word _edata
/* start address for the .bss section */
.word _sbss
/* end address for the .bss section */
.word _ebss

.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  ldr sp, =_estack     /* set stack pointer */

/* Copy data segment initializers from flash to SRAM */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4

LoopCopyDataInit:
  adds r4, r0, r3
  cmp r4, r1
  bcc CopyDataInit
  
/* Zero fill the bss segment */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str r3, [r2]
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4
  bcc FillZerobss

/* Call system init and main */
  bl SystemInit
  bl main

Infinite_Loop:
  b Infinite_Loop

.size Reset_Handler, .-Reset_Handler

/* Default handler */
.section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop_Default:
  b Infinite_Loop_Default
.size Default_Handler, .-Default_Handler

/* Vector Table */
.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word	MemManage_Handler
  .word	BusFault_Handler
  .word	UsageFault_Handler
  .word	0
  .word	0
  .word	0
  .word	0
  .word	SVC_Handler
  .word	DebugMon_Handler
  .word	0
  .word	PendSV_Handler
  .word	SysTick_Handler
  .word	0  /* External Interrupts */
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	TIM1_CC_IRQHandler      /* TIM1 Capture Compare */
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	0
  .word	DMA2_Stream1_IRQHandler /* DMA2 Stream 1 */

/* Weak aliases for exception handlers */
.weak NMI_Handler
.thumb_set NMI_Handler,Default_Handler

.weak HardFault_Handler
.thumb_set HardFault_Handler,Default_Handler

.weak MemManage_Handler
.thumb_set MemManage_Handler,Default_Handler

.weak BusFault_Handler
.thumb_set BusFault_Handler,Default_Handler

.weak UsageFault_Handler
.thumb_set UsageFault_Handler,Default_Handler

.weak SVC_Handler
.thumb_set SVC_Handler,Default_Handler

.weak DebugMon_Handler
.thumb_set DebugMon_Handler,Default_Handler

.weak PendSV_Handler
.thumb_set PendSV_Handler,Default_Handler

.weak SysTick_Handler
.thumb_set SysTick_Handler,Default_Handler

.weak TIM1_CC_IRQHandler
.thumb_set TIM1_CC_IRQHandler,Default_Handler

.weak DMA2_Stream1_IRQHandler
.thumb_set DMA2_Stream1_IRQHandler,Default_Handler
EOF

    echo "Created: $STARTUP_DIR/startup_stm32f411xe_minimal.s"
    
    echo "Creating minimal linker script..."
    
    cat > "$LINKER_DIR/STM32F411xE_minimal.ld" << 'EOF'
/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = 0x20020000;    /* end of RAM (128K) */

/* Memory regions */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 512K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 128K
}

/* Sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
}
EOF

    echo "Created: $LINKER_DIR/STM32F411xE_minimal.ld"
    
    # Update Makefile to use minimal files
    echo ""
    echo "Updating Makefile to use minimal files..."
    sed -i 's/startup_stm32f411xe.s/startup_stm32f411xe_minimal.s/g' "$PROJECT_DIR/Makefile" 2>/dev/null || \
    sed -i '' 's/startup_stm32f411xe.s/startup_stm32f411xe_minimal.s/g' "$PROJECT_DIR/Makefile"
    
    sed -i 's/STM32F411xE.ld/STM32F411xE_minimal.ld/g' "$PROJECT_DIR/Makefile" 2>/dev/null || \
    sed -i '' 's/STM32F411xE.ld/STM32F411xE_minimal.ld/g' "$PROJECT_DIR/Makefile"
    
    echo ""
    echo "✓ Minimal files created successfully!"
    echo ""
    echo "You can now build the project with: make"
fi

echo ""
echo "========================================="
echo "Setup complete!"
echo "========================================="
echo ""
echo "Next steps:"
echo "1. Review hardware configuration in inc/dshot.h"
echo "2. Connect your STM32 board and ST-Link"
echo "3. Build: make"
echo "4. Flash: make flash"
echo "5. Open serial terminal at 115200 baud"
echo ""
