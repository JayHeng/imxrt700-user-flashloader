# flashloader_rt700 (i.MX RT700 / MIMXRT798S CM33 core0) - UART-only SRAM flashloader

## Overview

This is a trimmed NXP MCU Bootloader (flashloader) example for the
**i.MX RT700 (MIMXRT798S)** Cortex-M33 core0. It is built to do exactly one job:

- Talk to the PC host tool **blhost** over a **UART**, and
- **download data into the chip's internal SRAM** (write-memory / read-memory /
  fill-memory), using the standard kboot serial framing protocol.

Everything not needed for that path has been removed compared to the RT1180
flashloader: no FlexSPI / SEMC / SD / eMMC / NAND drivers, no USB-HID, no
EdgeLock / keyblob, no SB file loader, and no autobaud.

The image runs entirely from on-chip SRAM (`BL_TARGET_RAM`). It is intended to
be loaded by the ROM serial downloader (SDP) and then executed, after which the
host switches to the flashloader command channel.

## Serial connection (blhost)

- Peripheral: **FLEXCOMM0 / LPUART0** (the RT700-EVK debug UART, pins PIO0_31 =
  RX/P0, PIO1_0 = TX/P1).
- Baud rate: **fixed 115200 8N1** (autobaud is NOT used - RT700 has no LPIT).
  Change `BL_FEATURE_UART_FIXED_BAUD` in
  `middleware/mcu_bootloader/targets/MIMXRT798S/src/bootloader_config.h` if you
  need a different rate.

Example host commands:

```
blhost -p COMx,115200 -- get-property 1
blhost -p COMx,115200 -- write-memory 0x20200000 my_data.bin
blhost -p COMx,115200 -- read-memory  0x20200000 256 dump.bin
blhost -p COMx,115200 -- fill-memory  0x20200000 0x100 0xA5A5A5A5
```

## SRAM download windows

Declared in
`middleware/mcu_bootloader/targets/MIMXRT798S/src/memory_config.h`
(system-bus view):

| Window | Range                     | Purpose                              |
|--------|---------------------------|--------------------------------------|
| 0      | 0x2008_0000 - 0x2017_FFFF | general SRAM (flashloader text alias)|
| 1      | 0x2018_0000 - 0x201F_FFFF | flashloader data / stack             |
| 2      | 0x2020_0000 - 0x2057_FFFF | free download target area            |

Use window 2 (`0x20200000`) as the safe host download target; it does not
overlap the flashloader image (which is linked at code-bus `0x0008_0000`,
data at `0x2018_0000`).

## Build (IAR EWARM)

Open `cm33_core0/iar/flashloader_cm33.eww` and build the `debug` or `release`
configuration. Output: `flashloader_cm33.out` / `flashloader_cm33.bin`.

## Files added for this port

Board / project (this folder):
- `iar/flashloader_cm33.ewp` / `.ewd` / `.eww`  - IAR project
- `iar/MIMXRT798S_flashloader_ram.icf`          - SRAM linker file
- `board.c/.h`, `clock_config.c/.h`, `pin_mux.c/.h`, `hardware_init.c`,
  `mcux_config.h`, `mcuxsdk_version.h`           - reused from hello_world_rt700

RT700 bootloader target port
(`middleware/mcu_bootloader/targets/MIMXRT798S/src/`):
- `bootloader_config.h`                - feature switches (UART+SRAM only)
- `target_config.h`                    - version / memory index constants
- `memory_config.h`                    - SRAM window addresses
- `memory_map_MIMXRT798S.c`            - g_memoryMap (SRAM, normal-memory iface)
- `periph_MIMXRT798S.c`                - g_peripherals (LPUART0 only)
- `lpuart_fixed_interface_MIMXRT798S.c`- fixed-baud LPUART peripheral interface
- `clock_cfg_MIMXRT798S.c`             - configure_clocks / get_uart_clock
- `microseconds_dwt_MIMXRT798S.c`      - DWT-based microseconds timebase
- `hardware_init_MIMXRT798S.c`         - init_hardware, UART pinmux, LP_FLEXCOMM0 ISR

## Notes / limitations (please review before production use)

1. **Fixed baud, no autobaud.** The stock kboot autobaud needs GPIO edge timing
   plus an LPIT; RT700 has no LPIT, so this port uses a fixed 115200 baud. The
   host must open the port at the same rate.
2. **Microseconds uses DWT** cycle counter (core-clock based) instead of LPIT.
3. **SRAM window addresses are conservative** placeholders. Confirm the exact
   SRAM partitioning / ROM-reserved regions against the i.MX RT700 reference
   manual and adjust `memory_config.h` and the `.icf` accordingly.
4. This is a functional scaffold generated from the RT1180 flashloader plus the
   hello_world_rt700 SDK project. It should be compiled with IAR and validated
   on hardware (a JLink connection can also be used to load and run the image
   before wiring up the ROM/SDP load path).
