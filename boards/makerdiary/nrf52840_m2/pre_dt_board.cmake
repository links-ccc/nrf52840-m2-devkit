# SPDX-License-Identifier: Apache-2.0

# Suppress overlaps which are expected in Nordic nRF52 SoC DTS files.
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
