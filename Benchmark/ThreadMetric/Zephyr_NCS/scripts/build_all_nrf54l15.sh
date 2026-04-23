#!/usr/bin/env bash
set -euo pipefail

BOARD="nrf54l15dk/nrf54l15/cpuapp"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_ROOT="$ROOT_DIR/build"

projects=(
  ThreadMetricBenchmarkZephyr_BasicProcessing_nRF54L15
  ThreadMetricBenchmarkZephyr_CooperativeScheduling_nRF54L15
  ThreadMetricBenchmarkZephyr_InterruptProcessing_nRF54L15
  ThreadMetricBenchmarkZephyr_InterruptPreemptionProcessing_nRF54L15
  ThreadMetricBenchmarkZephyr_MemoryAllocation_nRF54L15
  ThreadMetricBenchmarkZephyr_MessageProcessing_nRF54L15
  ThreadMetricBenchmarkZephyr_PreemptiveScheduling_nRF54L15
  ThreadMetricBenchmarkZephyr_SynchronizationProcessing_nRF54L15
)

mkdir -p "$BUILD_ROOT"

for project in "${projects[@]}"; do
  echo "==> Building $project for $BOARD"
  west build -p always -b "$BOARD" "$ROOT_DIR/$project" -d "$BUILD_ROOT/$project"
done

echo
echo "All builds finished under: $BUILD_ROOT"
