/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Bootloader (flashloader) feature configuration for i.MX RT700 (MIMXRT798S)
 * CM33 core0. This variant is trimmed to the minimum needed for a
 * UART <-> blhost host connection that downloads data into internal SRAM:
 *   - Only the LPUART (LP_FLEXCOMM) peripheral is enabled.
 *   - Only the "normal memory" (internal SRAM) interface is enabled.
 *   - No internal flash, no external memory (FlexSPI/SEMC/SD/MMC/NAND),
 *     no USB, no keyblob / EdgeLock, no SB file loader.
 */
#ifndef __BOOTLOADER_CONFIG_H__
#define __BOOTLOADER_CONFIG_H__

////////////////////////////////////////////////////////////////////////////////
// Bootloader Mode Configurations
////////////////////////////////////////////////////////////////////////////////
// Support ISP boot via serial peripheral.
#define BL_FEATURE_ISP_BOOT (1)

////////////////////////////////////////////////////////////////////////////////
// Bootloader Peripheral Configurations
////////////////////////////////////////////////////////////////////////////////
// UART port. RT700 debug/console FLEXCOMM0 (LPUART0) is used as the ISP UART.
#define BL_FEATURE_ROM_UART_PORT (1)

#if !defined(BL_CONFIG_LPUART_0)
#define BL_CONFIG_LPUART_0 (BL_FEATURE_ROM_UART_PORT)
#endif

#define BL_CONFIG_LPUART (BL_CONFIG_LPUART_0)

// UART autobaud via GPIO edge timing (PINT + INPUTMUX) driven by the DWT-based
// microseconds timebase. This lets the host pick the baud rate, e.g.
//   blhost -p COMx  (auto)   or   blhost -p COMx,115200
#define BL_FEATURE_UART_AUTOBAUD_IRQ (1)

// No SPI / USB in this trimmed build.
#define BL_CONFIG_LPSPI (0)
#define BL_CONFIG_HS_USB_HID (0)

////////////////////////////////////////////////////////////////////////////////
// Bootloader Feature Configurations
////////////////////////////////////////////////////////////////////////////////
#if !defined(BL_TARGET_FLASH) && !defined(BL_TARGET_RAM)
#define BL_TARGET_RAM (1)
#endif

#define BL_FEATURE_MIN_PROFILE (1)

// RAM target: no application CRC check / no jump-to-application.
#if !defined(BL_TARGET_RAM)
#define BL_FEATURE_CRC_CHECK (1)
#endif

// Peripheral detection timeout in milliseconds. 0 == wait forever (no jump).
#define BL_DEFAULT_PERIPHERAL_DETECT_TIMEOUT (0)

////////////////////////////////////////////////////////////////////////////////
// Internal Memory Module Configurations
////////////////////////////////////////////////////////////////////////////////
// No internal flash on this device family from the flashloader's perspective.
#define BL_FEATURE_HAS_NO_INTERNAL_FLASH (1)
#define BL_FEATURE_HAS_INTERNAL_FLASH (0)

// No EdgeLock / eFuse / keyblob services in this trimmed build.
#define BL_FEATURE_EFUSE_MODULE (0)
#define BL_FEATURE_EDGELOCK_MODULE (0)
#define BL_FEATURE_GEN_KEYBLOB (0)

////////////////////////////////////////////////////////////////////////////////
// External Memory Module Configurations (all disabled)
////////////////////////////////////////////////////////////////////////////////
#define BL_FEATURE_FLEXSPI_NOR_MODULE (0)
#define BL_FEATURE_SEMC_NOR_MODULE (0)
#define BL_FEATURE_EXPAND_MEMORY (0)
#define BL_FEATURE_SPINAND_MODULE (0)
#define BL_FEATURE_MMC_MODULE (0)
#define BL_FEATURE_SD_MODULE (0)
#define BL_FEATURE_SEMC_NAND_MODULE (0)
#define BL_FEATURE_SPI_NOR_EEPROM_MODULE (0)

////////////////////////////////////////////////////////////////////////////////
// Protocol Configurations
////////////////////////////////////////////////////////////////////////////////
#define BL_FEATURE_EXPAND_PACKET_SIZE (1)
#define BL_EXPANDED_FRAMING_PACKET_SIZE (512)

////////////////////////////////////////////////////////////////////////////////
// Un-Categoried Configurations
////////////////////////////////////////////////////////////////////////////////
#define BL_FETAURE_USE_STD_EXCEPTION_HANDLER (0)

// Do not support receive-SB-file command in this trimmed build.
#define BL_FETAURE_RECV_SB (0)

#endif // __BOOTLOADER_CONFIG_H__
