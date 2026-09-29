/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Board / hardware bring-up, UART pinmux and LP_FLEXCOMM IRQ glue for the
 * i.MX RT700 (MIMXRT798S) CM33 core0 UART-only SRAM flashloader.
 */

#include "bootloader.h"
#include "bootloader_common.h"
#include "bl_context.h"
#include "bl_peripheral_interface.h"
#include "fsl_device_registers.h"
#include "fsl_lpuart.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

////////////////////////////////////////////////////////////////////////////////
// External references
////////////////////////////////////////////////////////////////////////////////

// Defined in lpuart_fixed_interface_MIMXRT798S.c
extern void lpuart_fixed_rx_isr(uint32_t instance);

// The ISP UART instance used by this flashloader (FLEXCOMM0 -> LPUART0).
#define BL_ISP_UART_INSTANCE (0U)
#define BL_ISP_UART_IRQn     LP_FLEXCOMM0_IRQn

////////////////////////////////////////////////////////////////////////////////
// UART pinmux
////////////////////////////////////////////////////////////////////////////////

//! @brief Pinmux configuration hook used by the LPUART peripheral interface.
//!
//! The FLEXCOMM0 TX/RX pins (PIO0_31 / PIO1_0) are configured once by
//! BOARD_InitPins() during init_hardware(). All pinmux phases below are
//! satisfied by that single configuration, so we simply (re)apply it.
void uart_pinmux_config(uint32_t instance, pinmux_type_t pinmux)
{
    (void)instance;

    switch (pinmux)
    {
        case kPinmuxType_PollForActivity:
        case kPinmuxType_Peripheral:
            BOARD_InitPins();
            break;
        case kPinmuxType_Default:
        default:
            // Leave pins as-is; nothing to restore for this simple demo.
            break;
    }
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

//! @brief LP_FLEXCOMM0 interrupt handler.
//!
//! RT700 routes the LPUART RX/TX interrupt through the shared LP_FLEXCOMM
//! vector. Forward RX-full events to the flashloader byte receive callback.
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
    // Nothing extra to tear down for this UART-only SRAM flashloader.
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
