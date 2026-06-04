/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#include <m2devkit/soc.h>

#include <zephyr/sys/util.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_BOARD_NRF52840_M2),
	     "This application SoC layer is for nrf52840_m2 only");
BUILD_ASSERT(IS_ENABLED(CONFIG_SOC_NRF52840),
	     "This application SoC layer expects Nordic nRF52840");

static const struct m2devkit_soc_info soc_info = {
	.board = CONFIG_BOARD,
	.target = CONFIG_BOARD_TARGET,
	.soc = CONFIG_SOC,
	.soc_series = CONFIG_SOC_SERIES,
	.soc_family = CONFIG_SOC_FAMILY,
};

const struct m2devkit_soc_info *m2devkit_soc_info_get(void)
{
	return &soc_info;
}
