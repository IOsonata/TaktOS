# SPDX-License-Identifier: Apache-2.0
#
# Zephyr project for Thread-Metric Message Processing benchmark.
#
# Build (selects target MCU at build time):
#   west build -b nrf52dk/nrf52832          ThreadMetric/Zephyr/Zephyr_MessageProcessing
#   west build -b nrf54l15dk/nrf54l15/cpuapp ThreadMetric/Zephyr/Zephyr_MessageProcessing
#
# Sources pulled from ThreadMetric/src and ThreadMetric/include  the same
# C files used by the FreeRTOS / TaktOS / ThreadX Eclipse projects, ensuring
# apples-to-apples workload across all RTOSes.
#
# Single source of truth for Zephyr config: ../common/prj.conf  edit there,
# all 7 projects pick up the change. CONF_FILE must be set BEFORE
# find_package(Zephyr) to override the default prj.conf-next-to-CMakeLists.

cmake_minimum_required(VERSION 3.20.0)

set(CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../common/prj.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(Zephyr_MessageProcessing)

# Path to the shared ThreadMetric tree (../../ from this project).
set(TM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

target_include_directories(app PRIVATE
    ${TM_ROOT}/include
)

target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/main.c
    ${TM_ROOT}/src/tm_port_zephyr.c
    ${TM_ROOT}/src/tm_report_zephyr.c
    ${TM_ROOT}/src/message_processing.c
)
