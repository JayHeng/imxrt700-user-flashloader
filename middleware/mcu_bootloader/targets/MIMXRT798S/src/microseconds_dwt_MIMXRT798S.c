/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Microseconds timebase for the i.MX RT700 (MIMXRT798S) CM33 core0 UART-only
 * SRAM flashloader.
 *
 * The stock mcu_bootloader microseconds driver is LPIT based. RT700 has no
 * LPIT, so this implementation uses the Cortex-M33 DWT cycle counter which is
 * clocked at the core frequency. This is sufficient for the coarse delays and
 * (unused) peripheral-detect timeout in this UART-only build.
 */

#include "microseconds.h"
#include "bootloader_common.h"
#include "fsl_device_registers.h"

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

static uint32_t s_ticksPerMicrosecond = 1;
static uint64_t s_delayTargetTicks = 0;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

//! @brief Gets the clock value used for the microseconds driver (core clock).
uint32_t microseconds_get_clock(void)
{
    return SystemCoreClock;
}

//! @brief Initialize the DWT cycle counter as a free-running timebase.
void microseconds_init(void)
{
    SystemCoreClockUpdate();

    s_ticksPerMicrosecond = SystemCoreClock / 1000000u;
    if (s_ticksPerMicrosecond == 0u)
    {
        s_ticksPerMicrosecond = 1u;
    }

    // Enable the trace subsystem and the DWT cycle counter.
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

//! @brief Shutdown the microseconds timer.
void microseconds_shutdown(void)
{
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
}

//! @brief Read the running tick count (core cycles).
uint64_t microseconds_get_ticks(void)
{
    return (uint64_t)DWT->CYCCNT;
}

//! @brief Convert ticks to microseconds.
uint32_t microseconds_convert_to_microseconds(uint32_t ticks)
{
    return ticks / s_ticksPerMicrosecond;
}

//! @brief Convert microseconds to ticks.
uint64_t microseconds_convert_to_ticks(uint32_t microseconds)
{
    return (uint64_t)microseconds * (uint64_t)s_ticksPerMicrosecond;
}

//! @brief Busy-wait the given number of microseconds.
void microseconds_delay(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t target = us * s_ticksPerMicrosecond;
    while ((uint32_t)(DWT->CYCCNT - start) < target)
    {
        ; // busy wait
    }
}

//! @brief Arm a delay checked by microseconds_timeout().
void microseconds_set_delay(uint32_t us)
{
    s_delayTargetTicks = microseconds_get_ticks() + microseconds_convert_to_ticks(us);
}

//! @brief Return true once the armed delay has elapsed.
bool microseconds_timeout(void)
{
    return (microseconds_get_ticks() >= s_delayTargetTicks);
}

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
