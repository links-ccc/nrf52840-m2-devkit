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
#define DEMO_SQUARE_BORDER_WIDTH 3U
#define DEMO_SQUARE_X_STEP 5U
#define DEMO_SQUARE_Y_STEP 3U
#define DEMO_BACKGROUND_STEP 4U
#define DEMO_STRIPE_STEP 2U
#define DEMO_STRIPE_PERIOD 48U
#define DEMO_STRIPE_HEIGHT 4U
#define DEMO_BACKLIGHT_PERCENT 85U
#define DEMO_FRAME_DELAY_MS 40U
#define DEMO_WRITE_RETRY_DELAY_MS 500U

#define DISPLAY_ORIGIN_X 0U

#define PWM_PERCENT_MAX 100U

#define RGB565_RED_MASK 0xf8U
#define RGB565_GREEN_MASK 0xfcU
#define RGB565_RED_SHIFT 8U
#define RGB565_GREEN_SHIFT 3U
#define RGB565_BLUE_SHIFT 3U

#define RGB_CHANNEL_OFF 0U
#define RGB_CHANNEL_MAX 255U
#define COLOR_WHEEL_SEGMENT_SIZE 64U
#define COLOR_WHEEL_SEGMENT_COUNT 3U
#define COLOR_WHEEL_PERIOD (COLOR_WHEEL_SEGMENT_SIZE * COLOR_WHEEL_SEGMENT_COUNT)
#define COLOR_WHEEL_SECOND_SEGMENT_START COLOR_WHEEL_SEGMENT_SIZE
#define COLOR_WHEEL_THIRD_SEGMENT_START (COLOR_WHEEL_SEGMENT_SIZE * 2U)
#define COLOR_WHEEL_CHANNEL_STEP 4U
#define COLOR_WHEEL_LOW_RED 16U
#define COLOR_WHEEL_LOW_GREEN 40U
#define COLOR_WHEEL_LOW_BLUE 32U
#define DEMO_STRIPE_GREEN 230U
#define DEMO_STRIPE_BLUE 32U

static const struct device *const display = DEVICE_DT_GET(DISPLAY_NODE);
static const struct pwm_dt_spec backlight = PWM_DT_SPEC_GET(BACKLIGHT_NODE);
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(LED_NODE, gpios, {0});
static uint16_t line_buf[DEMO_MAX_WIDTH * DEMO_LINE_COUNT];

static uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
	return ((uint16_t)(red & RGB565_RED_MASK) << RGB565_RED_SHIFT) |
	       ((uint16_t)(green & RGB565_GREEN_MASK) << RGB565_GREEN_SHIFT) |
	       ((uint16_t)blue >> RGB565_BLUE_SHIFT);
}

static uint16_t color_wheel(uint16_t position)
{
	position %= COLOR_WHEEL_PERIOD;

	if (position < COLOR_WHEEL_SEGMENT_SIZE) {
		return rgb565(RGB_CHANNEL_MAX - (position * COLOR_WHEEL_CHANNEL_STEP),
			      position * COLOR_WHEEL_CHANNEL_STEP,
			      COLOR_WHEEL_LOW_BLUE);
	}

	if (position < COLOR_WHEEL_THIRD_SEGMENT_START) {
		position -= COLOR_WHEEL_SECOND_SEGMENT_START;
		return rgb565(COLOR_WHEEL_LOW_RED,
			      RGB_CHANNEL_MAX - (position * COLOR_WHEEL_CHANNEL_STEP),
			      position * COLOR_WHEEL_CHANNEL_STEP);
	}

	position -= COLOR_WHEEL_THIRD_SEGMENT_START;
	return rgb565(position * COLOR_WHEEL_CHANNEL_STEP,
		      COLOR_WHEEL_LOW_GREEN,
		      RGB_CHANNEL_MAX - (position * COLOR_WHEEL_CHANNEL_STEP));
}

static int set_backlight(uint8_t percent)
{
	uint32_t pulse = (uint64_t)backlight.period * MIN(percent, PWM_PERCENT_MAX) /
			 PWM_PERCENT_MAX;

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
	       (x < (square_x + DEMO_SQUARE_BORDER_WIDTH) ||
		x >= (square_x + DEMO_SQUARE_SIZE - DEMO_SQUARE_BORDER_WIDTH) ||
		y < (square_y + DEMO_SQUARE_BORDER_WIDTH) ||
		y >= (square_y + DEMO_SQUARE_SIZE - DEMO_SQUARE_BORDER_WIDTH));
}

static int draw_frame(uint16_t width, uint16_t height, uint16_t frame)
{
	struct display_buffer_descriptor desc = {
		.width = width,
		.pitch = width,
		.buf_size = sizeof(line_buf),
	};
	uint16_t square_x = (frame * DEMO_SQUARE_X_STEP) % (width - DEMO_SQUARE_SIZE);
	uint16_t square_y = (frame * DEMO_SQUARE_Y_STEP) % (height - DEMO_SQUARE_SIZE);

	for (uint16_t y = 0U; y < height; y += DEMO_LINE_COUNT) {
		uint16_t rows = MIN(DEMO_LINE_COUNT, height - y);

		desc.height = rows;
		desc.buf_size = width * rows * sizeof(line_buf[0]);

		for (uint16_t row = 0U; row < rows; row++) {
			for (uint16_t x = 0U; x < width; x++) {
				uint16_t abs_y = y + row;
				uint16_t color = color_wheel(x + abs_y + frame * DEMO_BACKGROUND_STEP);

				if (on_square_edge(x, abs_y, square_x, square_y)) {
					color = rgb565(RGB_CHANNEL_MAX, RGB_CHANNEL_MAX,
						       RGB_CHANNEL_MAX);
				} else if (inside_square(x, abs_y, square_x, square_y)) {
					color = rgb565(RGB_CHANNEL_OFF, RGB_CHANNEL_OFF,
						       RGB_CHANNEL_OFF);
				} else if (((abs_y + frame * DEMO_STRIPE_STEP) %
					    DEMO_STRIPE_PERIOD) < DEMO_STRIPE_HEIGHT) {
					color = rgb565(RGB_CHANNEL_MAX, DEMO_STRIPE_GREEN,
						       DEMO_STRIPE_BLUE);
				}

				line_buf[(row * width) + x] = color;
			}
		}

		int ret = display_write(display, DISPLAY_ORIGIN_X, y, &desc, line_buf);

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

	ret = set_backlight(DEMO_BACKLIGHT_PERCENT);
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
			k_sleep(K_MSEC(DEMO_WRITE_RETRY_DELAY_MS));
			continue;
		}

		k_sleep(K_MSEC(DEMO_FRAME_DELAY_MS));
	}

	return 0;
}
