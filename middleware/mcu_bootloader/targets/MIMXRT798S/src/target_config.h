/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Target constants for the i.MX RT700 (MIMXRT798S) CM33 core0 UART-only
 * SRAM flashloader.
 */
#if !defined(__TARGET_CONFIG_H__)
#define __TARGET_CONFIG_H__

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! @brief Unique ID constants.
enum _uid_constrants
{
    kUniqueId_SizeInBytes = 16,
};

//! @brief Version constants for the target.
enum _target_version_constants
{
    kTarget_Version_Name = 'T',
    kTarget_Version_Major = 1,
    kTarget_Version_Minor = 0,
    kTarget_Version_Bugfix = 0
};

//! @brief USB VID/PID (kept for property reporting compatibility, USB not built).
enum
{
    kProduct_USB_PID = 0x0073,
    kProduct_USB_VID = 0x15a2,
};

//! @brief Memory index definitions.
//!
//! property_imx.c refers to kIndexITCM / kIndexDTCM / kIndexOCRAM. In this
//! trimmed RT700 map we expose three internal SRAM windows in that order.
//! @brief Memory Map index constants
enum _special_memorymap_constants
{
    // kIndexFlashArray = 0,
    kIndexSRAM = 1,
    kIndexSRAMX = 2,
    kIndexSRAM1 = 3,
    kIndexSRAM2 = 4,
    kIndexSRAM3 = 5,

    kRAMSections = 5,

    // kSRAMSeparatrix = (uint32_t)0x20000000 //!< This value is the start address of SRAM_U
};

//! @brief i.MX RT700 SOC System ID.
#define IMXRT_SOC_SYSTEM_ID (0x1170UL)

#endif // __TARGET_CONFIG_H__
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
