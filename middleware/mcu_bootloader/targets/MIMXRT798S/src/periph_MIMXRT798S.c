/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Peripheral descriptor table for the i.MX RT700 (MIMXRT798S) CM33 core0
 * UART-only SRAM flashloader. Only the LPUART peripheral is exposed.
 */

#include "bl_context.h"
#include "bl_peripheral_interface.h"
#include "packet/serial_packet.h"

extern void uart_pinmux_config(uint32_t instance, pinmux_type_t pinmux);

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

#if !BL_CONFIG_LPUART
#error At least the LPUART peripheral must be enabled for this flashloader!
#endif

//! @brief Peripheral array for MIMXRT798S (UART-only).
const peripheral_descriptor_t g_peripherals[] = {
#if BL_CONFIG_LPUART_0
    // LPUART0 (FLEXCOMM0) - the debug/ISP UART on the RT700-EVK.
    { .typeMask = kPeripheralType_UART,
      .instance = 0,
      .pinmuxConfig = uart_pinmux_config,
      .controlInterface = &g_lpuartControlInterface,
      .byteInterface = &g_lpuartByteInterface,
      .packetInterface = &g_framingPacketInterface },
#endif // BL_CONFIG_LPUART_0

    { 0 } // Terminator
};

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
