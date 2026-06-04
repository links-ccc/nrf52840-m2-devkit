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

## Notes

The board keeps the original stock flash layout used by the factory Nordic MBR/nRF5 bootloader. The application slot starts at `0x10000`, matching the legacy project layout.

Nordic nRF HAL support is imported through Zephyr 4.4's official west manifest via `hal_nordic`.
