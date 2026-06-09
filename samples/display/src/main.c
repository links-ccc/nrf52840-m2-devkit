/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#define DISPLAY_NODE DT_CHOSEN(zephyr_display)
#define BACKLIGHT_NODE DT_ALIAS(backlight_pwm_led)
#define LED_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS(DISPLAY_NODE, okay)
#error "No ready zephyr,display chosen node"
#endif

#if !DT_NODE_HAS_STATUS(BACKLIGHT_NODE, okay)
#error "No ready backlight-pwm-led alias"
#endif

#define DEMO_LINE_COUNT 12U
#define DEMO_MAX_WIDTH 240U
#define DEMO_SQUARE_SIZE 42U

static const struct device *const display = DEVICE_DT_GET(DISPLAY_NODE);
static const struct pwm_dt_spec backlight = PWM_DT_SPEC_GET(BACKLIGHT_NODE);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(LED_NODE, gpios, {0});
static uint16_t line_buf[DEMO_MAX_WIDTH * DEMO_LINE_COUNT];

static uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((uint16_t)(red & 0xf8) << 8) |
	       ((uint16_t)(green & 0xfc) << 3) |
	       ((uint16_t)blue >> 3);
}

static uint16_t color_wheel(uint16_t position)
{
	position %= 192U;

	if (position < 64U) {
		return rgb565(255U - (position * 4U), position * 4U, 32U);
	}

	if (position < 128U) {
		position -= 64U;
		return rgb565(16U, 255U - (position * 4U), position * 4U);
	}

	position -= 128U;
	return rgb565(position * 4U, 40U, 255U - (position * 4U));
}

static int set_backlight(uint8_t percent)
{
	uint32_t pulse = (uint64_t)backlight.period * MIN(percent, 100U) / 100U;

	if (!pwm_is_ready_dt(&backlight)) {
		return -ENODEV;
	}

	return pwm_set_dt(&backlight, backlight.period, pulse);
}

static int init_status_led(void)
{
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	return gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}

static bool inside_square(uint16_t x, uint16_t y, uint16_t square_x, uint16_t square_y)
{
	return x >= square_x && x < (square_x + DEMO_SQUARE_SIZE) &&
	       y >= square_y && y < (square_y + DEMO_SQUARE_SIZE);
}

static bool on_square_edge(uint16_t x, uint16_t y, uint16_t square_x, uint16_t square_y)
{
	return inside_square(x, y, square_x, square_y) &&
	       (x < (square_x + 3U) || x >= (square_x + DEMO_SQUARE_SIZE - 3U) ||
		y < (square_y + 3U) || y >= (square_y + DEMO_SQUARE_SIZE - 3U));
}

static int draw_frame(uint16_t width, uint16_t height, uint16_t frame)
{
	struct display_buffer_descriptor desc = {
		.width = width,
		.pitch = width,
		.buf_size = sizeof(line_buf),
	};
	uint16_t square_x = (frame * 5U) % (width - DEMO_SQUARE_SIZE);
	uint16_t square_y = (frame * 3U) % (height - DEMO_SQUARE_SIZE);

	for (uint16_t y = 0U; y < height; y += DEMO_LINE_COUNT) {
		uint16_t rows = MIN(DEMO_LINE_COUNT, height - y);

		desc.height = rows;
		desc.buf_size = width * rows * sizeof(line_buf[0]);

		for (uint16_t row = 0U; row < rows; row++) {
			for (uint16_t x = 0U; x < width; x++) {
				uint16_t abs_y = y + row;
				uint16_t color = color_wheel(x + abs_y + frame * 4U);

				if (on_square_edge(x, abs_y, square_x, square_y)) {
					color = rgb565(255U, 255U, 255U);
				} else if (inside_square(x, abs_y, square_x, square_y)) {
					color = rgb565(0U, 0U, 0U);
				} else if (((abs_y + frame * 2U) % 48U) < 4U) {
					color = rgb565(255U, 230U, 32U);
				}

				line_buf[(row * width) + x] = color;
			}
		}

		int ret = display_write(display, 0, y, &desc, line_buf);

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int main(void)
{
	struct display_capabilities caps;
	uint16_t width;
	uint16_t height;
	uint16_t frame = 0U;
	int ret;

	printk("nRF52840 M.2 display sample\n");

	ret = init_status_led();
	if (ret < 0) {
		printk("Failed to initialize status LED: %d\n", ret);
		return 0;
	}

	if (!device_is_ready(display)) {
		printk("Display device is not ready\n");
		return 0;
	}

	ret = set_backlight(85U);
	if (ret < 0) {
		printk("Failed to enable display backlight: %d\n", ret);
		return 0;
	}

	display_get_capabilities(display, &caps);
	width = MIN(caps.x_resolution, DEMO_MAX_WIDTH);
	height = caps.y_resolution;

	printk("Display ready: %ux%u, pixel format 0x%x\n",
	       caps.x_resolution, caps.y_resolution, caps.current_pixel_format);

	ret = display_blanking_off(display);
	if (ret < 0) {
		printk("Failed to unblank display: %d\n", ret);
		return 0;
	}

	while (true) {
		if (gpio_is_ready_dt(&led)) {
			(void)gpio_pin_toggle_dt(&led);
		}

		ret = draw_frame(width, height, frame++);
		if (ret < 0) {
			printk("Display write failed: %d\n", ret);
			k_sleep(K_MSEC(500));
			continue;
		}

		k_sleep(K_MSEC(40));
	}

	return 0;
}
