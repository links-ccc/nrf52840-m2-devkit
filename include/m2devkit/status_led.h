/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef M2DEVKIT_STATUS_LED_H_
#define M2DEVKIT_STATUS_LED_H_

#include <stdbool.h>

int m2devkit_status_led_init(void);
int m2devkit_status_led_set(bool on);
int m2devkit_status_led_toggle(void);

#endif /* M2DEVKIT_STATUS_LED_H_ */
