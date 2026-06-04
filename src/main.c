/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <m2devkit/board.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	const struct m2devkit_soc_info *soc = m2devkit_soc_info_get();
	int ret;

	printk("nRF52840 M.2 DevKit running on %s (%s/%s)\n",
	       soc->target, soc->soc_family, soc->soc);

	ret = m2devkit_status_led_init();
	if (ret < 0) {
		printk("Failed to initialize status LED: %d\n", ret);
		return 0;
	}

	while (true) {
		(void)m2devkit_status_led_toggle();
		k_sleep(K_MSEC(1000));
	}

	return 0;
}
