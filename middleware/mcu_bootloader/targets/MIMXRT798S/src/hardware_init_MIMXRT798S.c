/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Board / hardware bring-up, UART pinmux, autobaud pin-IRQ glue and
 * LP_FLEXCOMM IRQ handler for the i.MX RT700 (MIMXRT798S) CM33 core0
 * UART-only SRAM flashloader.
 *
 * The ISP UART is FLEXCOMM0 / LPUART0 with:
 *   - RX = PIO0_31 (LP_FLEXCOMM0_P0), GPIO0 port 0 pin 31, func ALT1
 *   - TX = PIO1_0  (LP_FLEXCOMM0_P1), func ALT1
 *
 * For autobaud, the RX pin is temporarily muxed as a GPIO input and routed to
 * PINT (through INPUTMUX) so the autobaud edge detector can time the host's
 * 0x5A 0xA6 ping. Once autobaud finishes the pins are remuxed to UART mode.
 */

#include "bootloader.h"
#include "bootloader_common.h"
#include "bl_context.h"
#include "bl_peripheral_interface.h"
#include "fsl_device_registers.h"
#include "fsl_lpuart.h"
#include "fsl_gpio.h"
#include "fsl_pint.h"
#include "fsl_inputmux.h"
#include "fsl_clock.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

// Defined in lpuart_fixed_interface_MIMXRT798S.c
extern void lpuart_fixed_rx_isr(uint32_t instance);

// The ISP UART instance used by this flashloader (FLEXCOMM0 -> LPUART0).
#define BL_ISP_UART_INSTANCE (0U)
#define BL_ISP_UART_IRQn     LP_FLEXCOMM0_IRQn

// ISP UART RX pin (used as GPIO edge source during autobaud).
#define BL_UART_RX_PORT      (0U)
#define BL_UART_RX_PIN       (31U)
#define BL_UART_RX_FUNC_ALT  (1U) // LP_FLEXCOMM0_P0
#define BL_UART_TX_PORT      (1U)
#define BL_UART_TX_PIN       (0U)
#define BL_UART_TX_FUNC_ALT  (1U) // LP_FLEXCOMM0_P1

// PINT slot and INPUTMUX connection for the RX autobaud edge detection.
#define BL_UART_RX_PINT_BASE     PINT0
#define BL_UART_RX_PINT_TYPE     kPINT_PinInt0
#define BL_UART_RX_PINT_IRQn     PIN_INT0_IRQn
#define BL_UART_RX_PINT_IRQHDLR  PIN_INT0_IRQHandler
#define BL_UART_RX_INPUTMUX_SRC  kINPUTMUX_GpioPort0Pin31ToPintsel

#define GPIO_IRQC_INTERRUPT_ENABLED_PRIORITY  (1U)
#define GPIO_IRQC_INTERRUPT_RESTORED_PRIORITY (0U)

// IOPCTL field helpers (PIO[port][pin]).
#define IOPCTL_FSEL(x)   ((uint32_t)(x) & 0x7U)
#define IOPCTL_PUPDENA   (1U << 4)
#define IOPCTL_PUPDSEL   (1U << 5) // 1 = pull-up
#define IOPCTL_IBENA     (1U << 6)

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

//! Stores the autobaud pin-IRQ callback (single UART instance).
static pin_irq_callback_t s_pin_irq_func[1] = { 0 };

////////////////////////////////////////////////////////////////////////////////
// UART pinmux
////////////////////////////////////////////////////////////////////////////////

//! @brief Mux the RX pin as a plain GPIO input with pull-up (autobaud mode).
static void uart_rx_pin_to_gpio(void)
{
    IOPCTL0->PIO[BL_UART_RX_PORT][BL_UART_RX_PIN] =
        IOPCTL_FSEL(0) | IOPCTL_PUPDENA | IOPCTL_PUPDSEL | IOPCTL_IBENA;
    // Set the RX pin as a digital input.
    GPIO0->PDDR &= GPIO_FIT_REG(~(1UL << BL_UART_RX_PIN));
}

//! @brief Pinmux configuration hook used by the LPUART peripheral interface.
void uart_pinmux_config(uint32_t instance, pinmux_type_t pinmux)
{
    (void)instance;

    switch (pinmux)
    {
        case kPinmuxType_PollForActivity:
            // RX as GPIO input for autobaud edge detection.
            uart_rx_pin_to_gpio();
            break;
        case kPinmuxType_Peripheral:
            // RX/TX back to UART (LP_FLEXCOMM0) mode.
            IOPCTL0->PIO[BL_UART_RX_PORT][BL_UART_RX_PIN] = IOPCTL_FSEL(BL_UART_RX_FUNC_ALT) | IOPCTL_IBENA;
            IOPCTL0->PIO[BL_UART_TX_PORT][BL_UART_TX_PIN] = IOPCTL_FSEL(BL_UART_TX_FUNC_ALT) | IOPCTL_IBENA;
            break;
        case kPinmuxType_Default:
        default:
            IOPCTL0->PIO[BL_UART_RX_PORT][BL_UART_RX_PIN] = 0U;
            IOPCTL0->PIO[BL_UART_TX_PORT][BL_UART_TX_PIN] = 0U;
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Autobaud pin-IRQ (PINT + INPUTMUX)
////////////////////////////////////////////////////////////////////////////////

//! @brief PINT interrupt handler for the UART RX autobaud edge source.
void BL_UART_RX_PINT_IRQHDLR(void)
{
    // Clear the falling-edge detect flag.
    PINT_PinInterruptClrStatus(BL_UART_RX_PINT_BASE, BL_UART_RX_PINT_TYPE);

    if (s_pin_irq_func[0])
    {
        s_pin_irq_func[0](BL_ISP_UART_INSTANCE);
    }
    __DSB();
}

//! @brief Enable the autobaud RX pin falling-edge interrupt for the instance.
void enable_autobaud_pin_irq(uint32_t instance, pin_irq_callback_t func)
{
    (void)instance;

    // Make sure GPIO / PINT / INPUTMUX clocks are running.
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_InputMux0);
    CLOCK_EnableClock(kCLOCK_Pint0);

    NVIC_SetPriority(BL_UART_RX_PINT_IRQn, GPIO_IRQC_INTERRUPT_ENABLED_PRIORITY);
    NVIC_EnableIRQ(BL_UART_RX_PINT_IRQn);

    // Route the RX GPIO to the PINT selector, then arm falling-edge detection.
    INPUTMUX_Init(INPUTMUX0);
    INPUTMUX_AttachSignal(INPUTMUX0, BL_UART_RX_PINT_TYPE, BL_UART_RX_INPUTMUX_SRC);

    PINT_Init(BL_UART_RX_PINT_BASE);
    PINT_PinInterruptConfig(BL_UART_RX_PINT_BASE, BL_UART_RX_PINT_TYPE, kPINT_PinIntEnableFallEdge);

    s_pin_irq_func[0] = func;
}

//! @brief Disable the autobaud RX pin interrupt for the instance.
void disable_autobaud_pin_irq(uint32_t instance)
{
    (void)instance;

    NVIC_DisableIRQ(BL_UART_RX_PINT_IRQn);
    NVIC_SetPriority(BL_UART_RX_PINT_IRQn, GPIO_IRQC_INTERRUPT_RESTORED_PRIORITY);

    PINT_Deinit(BL_UART_RX_PINT_BASE);
    INPUTMUX_Deinit(INPUTMUX0);

    s_pin_irq_func[0] = 0;
}

////////////////////////////////////////////////////////////////////////////////
// LP_FLEXCOMM system IRQ control
////////////////////////////////////////////////////////////////////////////////

//! @brief Enable/disable the LP_FLEXCOMM (LPUART) NVIC interrupt.
void LPUART_SetSystemIRQ(uint32_t instance, PeripheralSystemIRQSetting set)
{
    (void)instance;

    if (set == kPeripheralEnableIRQ)
    {
        NVIC_EnableIRQ(BL_ISP_UART_IRQn);
    }
    else
    {
        NVIC_DisableIRQ(BL_ISP_UART_IRQn);
    }
}

//! @brief LP_FLEXCOMM0 interrupt handler (LPUART RX-full events).
void LP_FLEXCOMM0_IRQHandler(void)
{
    lpuart_fixed_rx_isr(BL_ISP_UART_INSTANCE);
    __DSB();
}

////////////////////////////////////////////////////////////////////////////////
// Hardware init / boot helpers
////////////////////////////////////////////////////////////////////////////////

void update_memory_map_lpc_sram(void)
{
}

//! @brief Initialize board hardware (clocks, pins, AHB secure controller).
void init_hardware(void)
{
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitAHBSC();

    // Attach FCCLK0 (OSC based) to FLEXCOMM0 so the ISP UART has a clock.
    CLOCK_AttachClk(BOARD_DEBUG_UART_FCCLK_ATTACH);
    CLOCK_SetClkDiv(BOARD_DEBUG_UART_FCCLK_DIV, 1U);
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    SystemCoreClockUpdate();
}

//! @brief Deinitialize board hardware.
void deinit_hardware(void)
{
}

//! @brief Boot pin is not used for the RAM flashloader.
bool is_boot_pin_asserted(void)
{
    return false;
}

//! @brief Flashloader always reports the NXP kboot VID/PID; nothing to update.
void update_available_peripherals(void)
{
}

//! @brief No fuse-based primary boot device selection in this trimmed build.
uint32_t get_primary_boot_device(void)
{
    return kBootDevice_Invalid;
}

//! @brief HAB/secure status is reported as open for this demo flashloader.
habstatus_option_t get_hab_status(void)
{
    return kHabStatus_Open;
}

#if __ICCARM__
//! @brief Redirect IAR low-level __write so library printf does not fault.
size_t __write(int handle, const unsigned char *buf, size_t size)
{
    (void)handle;
    (void)buf;
    return size;
}
#endif // __ICCARM__

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
