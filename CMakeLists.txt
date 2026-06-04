# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.5)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf52840_m2_devkit)

target_include_directories(app PRIVATE include)

add_subdirectory(soc)
add_subdirectory(drivers)

target_sources(app PRIVATE src/main.c)
