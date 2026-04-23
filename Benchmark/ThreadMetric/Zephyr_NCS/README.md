
## Open directly in VS Code with nRF Connect

This bundle now includes a ready-to-open VS Code workspace file:

- `Zephyr_NCS_ThreadMetric_Bundle_nRF54L15.code-workspace`

Expected flow:

1. Extract the zip anywhere outside your NCS tree.
2. In VS Code, use **File -> Open Workspace from File...**
3. Open `Zephyr_NCS_ThreadMetric_Bundle_nRF54L15.code-workspace`.
4. In the nRF Connect extension, the eight applications should already appear under **APPLICATIONS** because they are pre-listed in the workspace `nrf-connect.applications` setting.
5. For each application, create a build configuration for:
   - `nrf54l15dk/nrf54l15/cpuapp`

If your local nRF Connect extension does not auto-populate the app list immediately, reload the window once. The applications are still normal out-of-tree Zephyr/NCS apps and can also be added manually.

# Zephyr Thread-Metric bundle for nRF54L15 (NCS)

This bundle packages the current official Zephyr `tests/benchmarks/thread_metric`
source files into eight standalone NCS applications for the Nordic
`nrf54l15dk/nrf54l15/cpuapp` target.

The intent is practical: each benchmark is its own out-of-tree Zephyr app, so you can
build and flash it directly with `west` on nRF Connect SDK without editing the SDK tree.

Project set:

- ThreadMetricBenchmarkZephyr_BasicProcessing_nRF54L15
- ThreadMetricBenchmarkZephyr_CooperativeScheduling_nRF54L15
- ThreadMetricBenchmarkZephyr_InterruptProcessing_nRF54L15
- ThreadMetricBenchmarkZephyr_InterruptPreemptionProcessing_nRF54L15
- ThreadMetricBenchmarkZephyr_MemoryAllocation_nRF54L15
- ThreadMetricBenchmarkZephyr_MessageProcessing_nRF54L15
- ThreadMetricBenchmarkZephyr_PreemptiveScheduling_nRF54L15
- ThreadMetricBenchmarkZephyr_SynchronizationProcessing_nRF54L15

Notes:

- These wrappers vendor the current official Zephyr Thread-Metric source files under `common/src/`.
- The base `prj.conf` is copied from the official Zephyr benchmark and kept unchanged in each project.
- The upstream `testcase.yaml` and `thread_metric_readme.txt` are preserved under `upstream_snapshot/`.
- Zephyr's current official suite does **not** include mutex or mutex-barging tests. It includes
  interrupt and interrupt-preemption tests instead.
- The interrupt tests depend on Zephyr's `irq_offload()` support for the target/SDK revision.
- Console routing is left to the board/NCS defaults so you can use the console path already configured
  in your SDK setup.

## Build one project

Example:

```bash
west build -p always -b nrf54l15dk/nrf54l15/cpuapp       ThreadMetricBenchmarkZephyr_PreemptiveScheduling_nRF54L15
west flash
```

## Build all projects

From this bundle root:

```bash
./scripts/build_all_nrf54l15.sh
```

By default, build output goes to `./build/<project-name>/`.

## Capture output

Let the benchmark run for at least 30 seconds per interval and collect at least three intervals
if you want parity with Zephyr's Twister harness expectations.
