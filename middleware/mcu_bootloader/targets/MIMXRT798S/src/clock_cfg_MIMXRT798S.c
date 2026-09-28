/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Clock helpers for the i.MX RT700 (MIMXRT798S) CM33 core0 UART-only SRAM
 * flashloader. The heavy lifting (PLLs, XSPI, power) is done by the board
 * BOARD_BootClockRUN(); here we only expose the small helpers the bootloader
 * core and the LPUART interface need.
 */

#include "bootloader_common.h"
#include "fsl_clock.h"

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

//! @brief Configure hardware clocks.
//!
//! The RT700 board clock tree is already brought up by BOARD_BootClockRUN()
//! from init_hardware(). Here we only keep SystemCoreClock in sync.
void configure_clocks(bootloader_clock_option_t option)
{
    if (option == kClockOption_EnterBootloader)
    {
        SystemCoreClockUpdate();
    }
}

//! @brief Return the LPUART (LP_FLEXCOMM) source clock for the given instance.
//!
//! FLEXCOMM0 is attached to FCCLK0 in BOARD_InitDebugConsole(); this returns
//! the resulting LP_FLEXCOMM clock frequency used to compute the baud divider.
uint32_t get_uart_clock(uint32_t instance)
{
    return CLOCK_GetLPFlexCommClkFreq(instance);
}

//! @brief Bus clock (used by microseconds driver if needed).
uint32_t get_bus_clock(void)
{
    return SystemCoreClock;
}

//! @brief Core clock accessor.
uint32_t get_system_core_clock(void)
{
    return SystemCoreClock;
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
