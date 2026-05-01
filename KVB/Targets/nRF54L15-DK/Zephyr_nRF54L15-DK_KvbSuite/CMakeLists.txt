# SPDX-License-Identifier: MIT
#
# CMake configuration for the KVB benchmark suite on nRF54L15-DK with Zephyr.
#
# Build:
#
#   cd KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/
#   west build -p always -b nrf54l15dk/nrf54l15/cpuapp .
#   west flash
#
# The build pulls KVB source from KVB/src and KVB/ports/kernels/zephyr,
# the platform layer from KVB/Targets/nRF54L15-DK/src, and links it as a
# Zephyr application image.  Zephyr starts the kernel before main();
# main() then calls kvb_zephyr_start_default() which runs the KVB runner
# from the application thread (lifted to KVB_PRIORITY_HIGHEST by the
# Zephyr port).

cmake_minimum_required(VERSION 3.20.0)

# KVB_ROOT points at the KVB tree root.
# This CMakeLists.txt lives at: KVB/Targets/nRF54L15-DK/Zephyr_nRF54L15-DK_KvbSuite/
# So three levels up is the KVB root.
set(KVB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

# Per-target board+kernel platform implementation lives next to this file.
set(KVB_TARGET_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kvb_zephyr_nrf54l15dk LANGUAGES C CXX ASM)

# ----- KVB sources --------------------------------------------------------
# Core framework (kernel-agnostic).
target_sources(app PRIVATE
    ${KVB_ROOT}/src/core/kvb_default_tests.c
    ${KVB_ROOT}/src/core/kvb_log.c
    ${KVB_ROOT}/src/core/kvb_platform_default.c
    ${KVB_ROOT}/src/core/kvb_registry.c
    ${KVB_ROOT}/src/core/kvb_result.c
    ${KVB_ROOT}/src/core/kvb_runner.c
)

# Test bodies.
target_sources(app PRIVATE
    ${KVB_ROOT}/src/tests/sched/kvb_test_sched.c
    ${KVB_ROOT}/src/tests/sync/kvb_test_sync.c
    ${KVB_ROOT}/src/tests/ipc/kvb_test_ipc.c
    ${KVB_ROOT}/src/tests/time/kvb_test_time.c
)

# Zephyr kernel port.
target_sources(app PRIVATE
    ${KVB_ROOT}/ports/kernels/zephyr/kvb_port_zephyr.c
)

# Per-target platform layer (board ID strings, microsecond clock).
target_sources(app PRIVATE
    ${KVB_TARGET_ROOT}/src/kvb_platform_nrf54l15dk.c
)

# Application entry point — this directory's main.c.
target_sources(app PRIVATE
    src/main.c
)

# ----- Include paths ------------------------------------------------------
target_include_directories(app PRIVATE
    ${KVB_ROOT}/include
    ${KVB_ROOT}/ports/kernels/zephyr
    ${KVB_ROOT}/src/tests
    ${KVB_TARGET_ROOT}/include
)

# ----- KVB port + target identification ----------------------------------
target_compile_definitions(app PRIVATE
    KVB_PORT_ZEPHYR=1
    KVB_TARGET_NRF54L15DK=1
)

# ----- Per-target KVB knobs ----------------------------------------------
# kvb_config_nrf54l15dk_zephyr.h is force-included into every TU so the
# board-specific overrides (KVB_CORE_CLOCK_HZ, KVB_TICK_HZ, stack sizes,
# 10-second measurement window) land before kvb_config.h's defaults.
target_compile_options(app PRIVATE
    -include kvb_config_nrf54l15dk_zephyr.h
)
