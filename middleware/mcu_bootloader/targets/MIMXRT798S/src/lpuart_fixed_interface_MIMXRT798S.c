/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Autobaud-capable LPUART peripheral interface for the i.MX RT700 (MIMXRT798S)
 * CM33 core0 UART-only SRAM flashloader.
 *
 * This mirrors the stock mcu_bootloader lpuart_peripheral_interface.c and the
 * i.MX RT685 ram_flashloader implementation: the UART RX pin is first muxed as
 * a GPIO and routed to PINT (through INPUTMUX) so the autobaud detector can
 * measure the host's 0x5A 0xA6 ping edges. Once autobaud completes the LPUART
 * is initialized at the detected baud rate and normal RX-interrupt operation
 * begins.
 *
 * This file DEFINES g_lpuartControlInterface and g_lpuartByteInterface, so the
 * stock lpuart_peripheral_interface.c must NOT be added to the project.
 */

#include "bl_context.h"
#include "bootloader_common.h"
#include "bootloader_config.h"
#include "bl_peripheral_interface.h"
#include "autobaud.h"
#include "serial_packet.h"
#include "fsl_device_registers.h"
#include "fsl_lpuart.h"
#include "fsl_assert.h"

#if BL_CONFIG_LPUART

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

static bool lpuart_poll_for_activity(const peripheral_descriptor_t *self);
static status_t lpuart_full_init(const peripheral_descriptor_t *self, serial_byte_receive_func_t function);
static void lpuart_full_shutdown(const peripheral_descriptor_t *self);
static status_t lpuart_write(const peripheral_descriptor_t *self, const uint8_t *buffer, uint32_t byteCount);

extern void LPUART_SetSystemIRQ(uint32_t instance, PeripheralSystemIRQSetting set);

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

const peripheral_control_interface_t g_lpuartControlInterface = {
    .pollForActivity = lpuart_poll_for_activity,
    .init = lpuart_full_init,
    .shutdown = lpuart_full_shutdown,
    .pump = 0
};

const peripheral_byte_inteface_t g_lpuartByteInterface = { .init = NULL, .write = lpuart_write };

static serial_byte_receive_func_t s_lpuart_byte_receive_callback;

static bool g_lpuartInitStatus[FSL_FEATURE_SOC_LPUART_COUNT] = { false };

static const uint32_t g_lpuartBaseAddr[] = LPUART_BASE_ADDRS;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

/*!
 * @brief Drive autobaud detection for this UART instance.
 *
 * Called repeatedly from the peripheral-detection loop. Returns false while
 * autobaud is still measuring; once the rate is known the LPUART is fully
 * initialized, the pins are remuxed to peripheral mode, the RX interrupt is
 * enabled and the detected ping is injected into the framing layer.
 */
static bool lpuart_poll_for_activity(const peripheral_descriptor_t *self)
{
    uint32_t instance = self->instance;

    // Check for autobaud completion.
    uint32_t baud = 0;
    status_t autoBaudCompleted = autobaud_get_rate(instance, &baud);

    if (autoBaudCompleted == kStatus_Success)
    {
        lpuart_config_t userConfig;
        uint32_t baseAddr = g_lpuartBaseAddr[instance];

        LPUART_GetDefaultConfig(&userConfig);
        userConfig.baudRate_Bps = baud;
        userConfig.enableTx = true;
        userConfig.enableRx = true;

        if (LPUART_Init((LPUART_Type *)baseAddr, &userConfig, get_uart_clock(instance)) == kStatus_Success)
        {
            // Switch the RX/TX pins from the autobaud GPIO mode to UART mode.
            self->pinmuxConfig(instance, kPinmuxType_Peripheral);

            // Enable the LP_FLEXCOMM system interrupt and the RX-full interrupt.
            LPUART_SetSystemIRQ(instance, kPeripheralEnableIRQ);
            LPUART_EnableInterrupts((LPUART_Type *)baseAddr, kLPUART_RxDataRegFullInterruptEnable);

            LPUART_EnableRx((LPUART_Type *)baseAddr, true);
            LPUART_EnableTx((LPUART_Type *)baseAddr, true);

            // Feed the detected ping bytes to the command/framing layer.
            s_lpuart_byte_receive_callback(kFramingPacketStartByte);
            s_lpuart_byte_receive_callback(kFramingPacketType_Ping);

            g_lpuartInitStatus[instance] = true;

            // Autobaud is complete and the UART is active.
            return true;
        }
        else
        {
            // Init failed, restart autobaud.
            autobaud_init(instance);
        }
    }

    return false;
}

/*!
 * @brief Prepare the peripheral for autobaud activity detection.
 *
 * Muxes the RX pin as a GPIO routed to PINT and starts the autobaud edge
 * detector. The LPUART clock gate is not ungated until autobaud completes.
 */
static status_t lpuart_full_init(const peripheral_descriptor_t *self, serial_byte_receive_func_t function)
{
    s_lpuart_byte_receive_callback = function;

    // Configure the RX pin as a GPIO edge source for autobaud detection.
    self->pinmuxConfig(self->instance, kPinmuxType_PollForActivity);

    // Init the autobaud detector (installs the pin IRQ callback).
    autobaud_init(self->instance);

    return kStatus_Success;
}

//! @brief Shut down the peripheral and (optionally) restore pins.
static void lpuart_full_shutdown(const peripheral_descriptor_t *self)
{
    uint32_t instance = self->instance;

    if (g_lpuartInitStatus[instance])
    {
        uint32_t baseAddr = g_lpuartBaseAddr[instance];
        LPUART_SetSystemIRQ(instance, kPeripheralDisableIRQ);
        LPUART_Deinit((LPUART_Type *)baseAddr);
        g_lpuartInitStatus[instance] = false;
    }

#if BL_FEATURE_UART_AUTOBAUD_IRQ
    // De-init autobaud detector so the user app is not left with a stray IRQ.
    autobaud_deinit(instance);
#endif

    // Restore the pins to their default (reset) state.
    self->pinmuxConfig(self->instance, kPinmuxType_Default);
}

//! @brief Blocking write to the LPUART.
static status_t lpuart_write(const peripheral_descriptor_t *self, const uint8_t *buffer, uint32_t byteCount)
{
    uint32_t baseAddr = g_lpuartBaseAddr[self->instance];
    LPUART_WriteBlocking((LPUART_Type *)baseAddr, buffer, byteCount);
    return kStatus_Success;
}

//! @brief RX byte handler shared by the LP_FLEXCOMM ISR shim.
void lpuart_fixed_rx_isr(uint32_t instance)
{
    uint32_t baseAddr = g_lpuartBaseAddr[instance];
    LPUART_Type *base = (LPUART_Type *)baseAddr;

    if (LPUART_GetStatusFlags(base) & (uint32_t)kLPUART_RxDataRegFullFlag)
    {
        uint8_t byte = (uint8_t)LPUART_ReadByte(base);
        if (s_lpuart_byte_receive_callback != NULL)
        {
            s_lpuart_byte_receive_callback(byte);
        }
    }
}

#endif // BL_CONFIG_LPUART
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
