/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <m2devkit/status_led.h>

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#define STATUS_LED_NODE DT_ALIAS(led0)

#if !DT_NODE_HAS_STATUS_OKAY(STATUS_LED_NODE)
#error "nRF52840 M.2 DevKit requires an okay led0 alias"
#endif

static const struct gpio_dt_spec status_led =
	GPIO_DT_SPEC_GET(STATUS_LED_NODE, gpios);
static bool status_led_ready;

int m2devkit_status_led_init(void)
{
	int ret;

	if (status_led_ready) {
		return 0;
	}

	if (!gpio_is_ready_dt(&status_led)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	status_led_ready = true;
	return 0;
}

int m2devkit_status_led_set(bool on)
{
	int ret;

	ret = m2devkit_status_led_init();
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_set_dt(&status_led, on);
}

int m2devkit_status_led_toggle(void)
{
	int ret;

	ret = m2devkit_status_led_init();
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_toggle_dt(&status_led);
}
