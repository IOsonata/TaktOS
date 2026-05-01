# SPDX-License-Identifier: MIT
#
# CMake configuration for the KVB benchmark suite as a Zephyr application.
#
# Board-agnostic.  The MCU and board are selected at build time via the
# -b flag to west, the same way every other Zephyr application works:
#
#   west build -p always -b nrf54l15dk/nrf54l15/cpuapp .
#   west build -p always -b nrf52840dk/nrf52840 .
#   west build -p always -b stm32l4r9i_iot01a .
#   west build -p always -b qemu_cortex_m3 .
#
# The application source is identical for every board.  Per-board
# overrides (Kconfig fragments, devicetree overlays) live in boards/
# and are auto-picked-up by Zephyr's build system based on the
# selected board target.
#
# After build, flash with:
#
#   west flash
#
# Connect a serial terminal at 115200 8N1 to the board's default
# console (whatever zephyr,chosen zephyr,console points at).  KVB
# emits its log lines via printk; output appears immediately on reset.

cmake_minimum_required(VERSION 3.20.0)

# Locate the KVB tree.  This CMakeLists.txt lives at:
#   KVB/Targets/Zephyr/KvbSuite/
# So three levels up is KVB/.
set(KVB_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kvb_zephyr LANGUAGES C CXX ASM)

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

# Zephyr kernel port — provides KvbThread/Sem/Mutex/Queue using k_*.
target_sources(app PRIVATE
    ${KVB_ROOT}/ports/kernels/zephyr/kvb_port_zephyr.c
)

# Generic Zephyr platform layer — board ID strings, microsecond clock,
# log routing.  Uses only Zephyr APIs and CONFIG_BOARD/CONFIG_SOC, so
# this single file works for every board.
target_sources(app PRIVATE
    src/kvb_platform_zephyr.c
)

# Application entry — calls kvb_zephyr_start_default() from main().
target_sources(app PRIVATE
    src/main.c
)

# ----- Include paths ------------------------------------------------------
target_include_directories(app PRIVATE
    ${KVB_ROOT}/include
    ${KVB_ROOT}/ports/kernels/zephyr
    ${KVB_ROOT}/src/tests
    include
)

# ----- KVB port identification -------------------------------------------
target_compile_definitions(app PRIVATE
    KVB_PORT_ZEPHYR=1
)

# ----- KVB knobs ---------------------------------------------------------
# kvb_config_zephyr.h is force-included into every TU so the Zephyr-
# build defaults (10 s window, 1024 B worker stacks, 2048 B runner
# stack, 5 workers) land before kvb_config.h's internal defaults.
#
# Per-board overrides — if a particular board needs different KVB knob
# values, drop them as -D defines in boards/<board>.conf via a
# per-board CMakeLists.txt fragment, OR add board-conditional blocks
# inside kvb_config_zephyr.h gated on CONFIG_BOARD_<NAME>.
target_compile_options(app PRIVATE
    -include kvb_config_zephyr.h
)
