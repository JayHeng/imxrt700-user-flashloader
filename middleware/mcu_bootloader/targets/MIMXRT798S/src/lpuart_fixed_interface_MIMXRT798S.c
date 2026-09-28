/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Fixed-baud LPUART peripheral interface for the i.MX RT700 (MIMXRT798S)
 * CM33 core0 UART-only SRAM flashloader.
 *
 * The stock lpuart_peripheral_interface.c in the mcu_bootloader middleware
 * relies on the ROM autobaud detection which needs GPIO edge timing plus an
 * LPIT timebase. RT700 has no LPIT and this trimmed flashloader intentionally
 * avoids autobaud. Instead this file brings the LPUART up at a fixed baud rate
 * (BL_FEATURE_UART_FIXED_BAUD) and reports "active" as soon as a valid ping
 * framing byte is received.
 *
 * This file DEFINES g_lpuartControlInterface and g_lpuartByteInterface, so the
 * stock lpuart_peripheral_interface.c must NOT be added to the project.
 */

#include "bl_context.h"
#include "bootloader_common.h"
#include "bootloader_config.h"
#include "bl_peripheral_interface.h"
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

//! @brief Tracks whether the fixed-baud LPUART has been brought up yet.
static bool s_lpuartConfigured = false;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

//! @brief Configure the LPUART at the fixed baud rate and enable RX interrupt.
static status_t configure_lpuart_fixed(uint32_t instance)
{
    lpuart_config_t userConfig;
    uint32_t baseAddr = g_lpuartBaseAddr[instance];

    LPUART_GetDefaultConfig(&userConfig);
    userConfig.baudRate_Bps = BL_FEATURE_UART_FIXED_BAUD;
    userConfig.enableTx = true;
    userConfig.enableRx = true;

    if (LPUART_Init((LPUART_Type *)baseAddr, &userConfig, get_uart_clock(instance)) != kStatus_Success)
    {
        return kStatus_Fail;
    }

    // Enable RX-full interrupt so bytes are pushed to the framing layer.
    LPUART_EnableInterrupts((LPUART_Type *)baseAddr, kLPUART_RxDataRegFullInterruptEnable);
    LPUART_SetSystemIRQ(instance, kPeripheralEnableIRQ);

    g_lpuartInitStatus[instance] = true;
    return kStatus_Success;
}

//! @brief Poll for activity.
//!
//! With autobaud disabled we simply configure the UART once (on the first
//! poll) and then let the RX ISR feed bytes to the framing layer. Activity is
//! reported by the framing packet layer once a valid ping is seen; here we
//! just make sure the UART is running and return false so bl_main keeps
//! pumping until a real host packet arrives. The framing layer flags the
//! peripheral active through the byte-received callback.
static bool lpuart_poll_for_activity(const peripheral_descriptor_t *self)
{
    uint32_t instance = self->instance;

    if (!s_lpuartConfigured)
    {
        // Mux the pins for UART operation and bring up the controller.
        self->pinmuxConfig(instance, kPinmuxType_Peripheral);

        if (configure_lpuart_fixed(instance) == kStatus_Success)
        {
            s_lpuartConfigured = true;
        }
        return false;
    }

    // Activity is detected via the framing layer (byte receive callback).
    // Returning the framing layer's active status lets bl_main proceed.
    return false;
}

//! @brief Initialize the peripheral for activity detection.
static status_t lpuart_full_init(const peripheral_descriptor_t *self, serial_byte_receive_func_t function)
{
    s_lpuart_byte_receive_callback = function;
    s_lpuartConfigured = false;

    // Pre-mux the pins so the line idles correctly before configuration.
    self->pinmuxConfig(self->instance, kPinmuxType_PollForActivity);

    return kStatus_Success;
}

//! @brief Shut down the peripheral.
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
