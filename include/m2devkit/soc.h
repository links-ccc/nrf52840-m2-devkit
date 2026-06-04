/*
 * Copyright (c) 2026
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef M2DEVKIT_SOC_H_
#define M2DEVKIT_SOC_H_

struct m2devkit_soc_info {
	const char *board;
	const char *target;
	const char *soc;
	const char *soc_series;
	const char *soc_family;
};

const struct m2devkit_soc_info *m2devkit_soc_info_get(void);

#endif /* M2DEVKIT_SOC_H_ */
