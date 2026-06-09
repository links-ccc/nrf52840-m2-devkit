# Display Sample

Animated color-pattern sample for the nRF52840 M.2 Developer Kit 240x240 ST7789V TFT.

The board devicetree provides the ST7789V display as `zephyr,display` and the LCD backlight as the `backlight-pwm-led` alias. The sample enables the backlight, unblanks the display, then draws a moving RGB565 pattern with a scanning stripe and moving square.

Build from the repository root:

```sh
west build -d /private/tmp/m2devkit-display-sample-build -p always -b nrf52840_m2 samples/display
```

Flash with the onboard CMSIS-DAP probe:

```sh
west flash -d /private/tmp/m2devkit-display-sample-build --runner openocd
```
