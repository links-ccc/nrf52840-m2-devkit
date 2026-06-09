# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.5)

get_filename_component(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
list(APPEND BOARD_ROOT ${REPO_ROOT})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf52840_m2_display_sample)

target_sources(app PRIVATE src/main.c)
