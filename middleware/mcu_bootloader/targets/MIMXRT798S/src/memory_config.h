/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Internal SRAM memory-window definitions for the i.MX RT700 (MIMXRT798S)
 * CM33 core0 UART-only SRAM flashloader.
 *
 * RT700 has a large unified on-chip SRAM that is visible through two bus
 * views on the CM33:
 *   - Code bus  : 0x0000_0000 .. 0x0057_FFFF
 *   - System bus: 0x2000_0000 .. 0x2057_FFFF
 *
 * The ROM reserves the low SRAM for its own use during boot. The flashloader
 * itself is linked to run from 0x0008_0000 (see the .icf). The windows below
 * are the ranges the host is allowed to write into via blhost write-memory.
 * They intentionally EXCLUDE the region occupied by the flashloader image.
 */
#if !defined(__MEMORY_MAP_H__)
#define __MEMORY_MAP_H__

/* ======================== Size helpers ==================================== */
#define SIZE_1KB (0x00000400u)
#define SIZE_1MB (SIZE_1KB * 1024u)

/*
 * SRAM (System bus view) split into three windows so the property store can
 * report kIndexITCM / kIndexDTCM / kIndexOCRAM. All three are plain SRAM.
 *
 * Layout (System bus):
 *   0x2000_0000 .. 0x2007_FFFF : reserved for ROM / flashloader lower use
 *   0x2008_0000 .. 0x2017_FFFF : SRAM window 0 (flashloader text lives in the
 *                                code-bus alias 0x0008_0000.., host may target
 *                                the remaining data area from here)
 *   0x2018_0000 .. 0x201F_FFFF : SRAM window 1 (flashloader data / stack alias)
 *   0x2020_0000 .. 0x2057_FFFF : SRAM window 2 (free download target area)
 *
 * NOTE: These are conservative, aligned windows intended for a UART -> SRAM
 * download demo. Adjust to match the exact TRM partitioning for production.
 */

/* Window 0 : general SRAM (code+data capable). */
#define SRAM0_START_ADDRESS (0x20080000u)
#define SRAM0_END_ADDRESS   (0x2017FFFFu)

/* Window 1 : SRAM used by the flashloader for data/stack. */
#define SRAM1_START_ADDRESS (0x20180000u)
#define SRAM1_END_ADDRESS   (0x201FFFFFu)

/* Window 2 : free SRAM download target area. */
#define SRAM2_START_ADDRESS (0x20200000u)
#define SRAM2_END_ADDRESS   (0x2057FFFFu)

#endif // __MEMORY_MAP_H__
////////////////////////////////////////////////////////////////////////////////
// EOF
////////////////////////////////////////////////////////////////////////////////
