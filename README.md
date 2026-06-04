# nRF52840 M.2 Developer Kit - Zephyr 4.4 App

This repository is trimmed to the code needed to build a Zephyr application for the makerdiary nRF52840 M.2 Developer Kit.

It contains:

- A Zephyr 4.4 west manifest pinned to `zephyrproject-rtos/zephyr@v4.4.0`.
- An out-of-tree board definition for `nrf52840_m2`.
- A small layered application that prints a boot message and blinks `led0`.
- Project-owned `include/`, `drivers/`, and `soc/` trees for board-specific code.

The original examples, generated firmware images, documentation site, Python examples, nRF5 SDK examples, and USB driver packages have been removed.

## Layout

```text
boards/makerdiary/nrf52840_m2/  Zephyr board definition, DTS, pinctrl, partitions
include/m2devkit/               Public project headers
drivers/status_led/             Project-owned status LED driver wrapper
soc/                            Project SoC/board capability layer for nRF52840 M.2
src/                            Application entry point
```

The SoC silicon remains Zephyr's official `nrf52840` SoC. The local `soc/` directory is for this product's SoC-level checks and metadata, not a fork of Nordic's SoC port.

## Build

From the west workspace root:

```sh
west update
west zephyr-export
pip install -r zephyr/scripts/requirements.txt
west build -b nrf52840_m2 nrf52840-m2-devkit
```

Or from this repository:

```sh
west build -b nrf52840_m2 .
```

## Flash

The board has been verified with the Zephyr OpenOCD runner and the onboard CMSIS-DAP probe.

From the west workspace root:

```sh
source .venv/bin/activate
west build -d /private/tmp/m2devkit-local-flash-build -p always -b nrf52840_m2 nrf52840-m2-devkit
west flash -d /private/tmp/m2devkit-local-flash-build --runner openocd
```

The flash log should identify a CMSIS-DAP probe and an `nRF52840-xxAA` target, then write `zephyr/zephyr.hex`.

## Runtime smoke test

The application prints a boot message and toggles `led0` every 250 ms. It also increments the `m2devkit_smoke_heartbeat` RAM symbol in the main loop so runtime can be verified over SWD even when the UART console is not connected to the host.

Find the heartbeat symbol address:

```sh
/Users/links/zephyr-sdk-1.0.1/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-nm -n \
  /private/tmp/m2devkit-local-flash-build/zephyr/zephyr.elf | rg m2devkit_smoke_heartbeat
```

Read it after reset/run with the Zephyr SDK OpenOCD:

```sh
/Users/links/zephyr-sdk-1.0.1/hosttools/usr/bin/openocd \
  -s /Users/links/zephyr-sdk-1.0.1/hosttools/opt/openocd/share/openocd/scripts \
  -c 'set WORKAREASIZE 0x4000' \
  -c 'source [find interface/cmsis-dap.cfg]' \
  -c 'transport select swd' \
  -c 'source [find target/nrf52.cfg]' \
  -c 'init' \
  -c 'reset run' \
  -c 'sleep 1800' \
  -c 'halt' \
  -c 'mdw 0x20000280 1' \
  -c 'reset run' \
  -c 'shutdown'
```

A non-zero value at the heartbeat address confirms that the firmware entered the main loop. The hardware smoke test observed `0x20000280: 00000007` after about 1.8 seconds.

## Notes

The board keeps the original stock flash layout used by the factory Nordic MBR/nRF5 bootloader. The application slot starts at `0x10000`, matching the legacy project layout.

Nordic nRF HAL support is imported through Zephyr 4.4's official west manifest via `hal_nordic`.
