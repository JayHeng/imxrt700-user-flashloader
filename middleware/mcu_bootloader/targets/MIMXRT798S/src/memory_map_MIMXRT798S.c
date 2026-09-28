/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Memory map for the i.MX RT700 (MIMXRT798S) CM33 core0 UART-only SRAM
 * flashloader. Only internal SRAM windows are exposed; every window uses the
 * generic "normal memory" interface so blhost write-memory / read-memory /
 * fill-memory operate directly on SRAM.
 */

#include "bootloader.h"
#include "memory.h"
#include "memory_config.h"

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

//! @brief Memory map.
//!
//! Order matters: property_imx.c reads indices kIndexITCM(0), kIndexDTCM(1)
//! and kIndexOCRAM(2) which are aliased to the three SRAM windows below.
memory_map_entry_t g_memoryMap[] = {
    // SRAM window 0 (kIndexITCM alias) - general purpose SRAM.
    { .startAddress = SRAM0_START_ADDRESS,
      .endAddress = SRAM0_END_ADDRESS,
      .memoryProperty = kMemoryIsExecutable | kMemoryType_RAM,
      .memoryId = kMemoryInternal,
      .memoryInterface = &g_normalMemoryInterface },

    // SRAM window 1 (kIndexDTCM alias) - flashloader data/stack region.
    { .startAddress = SRAM1_START_ADDRESS,
      .endAddress = SRAM1_END_ADDRESS,
      .memoryProperty = kMemoryIsExecutable | kMemoryType_RAM,
      .memoryId = kMemoryInternal,
      .memoryInterface = &g_normalMemoryInterface },

    // SRAM window 2 (kIndexOCRAM alias) - free download target area.
    { .startAddress = SRAM2_START_ADDRESS,
      .endAddress = SRAM2_END_ADDRESS,
      .memoryProperty = kMemoryIsExecutable | kMemoryType_RAM,
      .memoryId = kMemoryInternal,
      .memoryInterface = &g_normalMemoryInterface },

    // Terminator
    { 0 }
};

////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
