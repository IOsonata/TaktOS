# Per-board overrides

This directory is automatically searched by Zephyr's build system when
you run `west build -b <board>` from the application root.  Files
matching the board target name are auto-merged with the application's
top-level configuration.

## File naming

For board target `nrf54l15dk/nrf54l15/cpuapp` the matching files are:

| File                                              | Effect                                           |
| ------------------------------------------------- | ------------------------------------------------ |
| `boards/nrf54l15dk_nrf54l15_cpuapp.conf`          | Kconfig overrides merged on top of `prj.conf`   |
| `boards/nrf54l15dk_nrf54l15_cpuapp.overlay`       | Devicetree overlay for hardware customisation   |
| `boards/nrf54l15dk_nrf54l15_cpuapp_defconfig`     | (rare) replaces the base board defconfig         |

The naming rule is "the board target name with `/` replaced by `_`,
plus the appropriate extension."  Zephyr documents the full lookup
order in the application development guide.

## When you need a per-board file

For most boards, the generic `prj.conf` and the empty `kvb_config_zephyr.h`
defaults are sufficient.  Add a per-board file only when:

1. **The board's defconfig overrides something KVB depends on.**
   For example, a board that defaults `CONFIG_SYS_CLOCK_TICKS_PER_SEC=100`
   would break KVB's 1 kHz timing assumptions; a per-board `.conf` can
   force it back to 1000 with `CONFIG_SYS_CLOCK_TICKS_PER_SEC=1000`.

2. **The board has tight RAM and the generic stack defaults overflow.**
   On a hypothetical 8 KB SRAM Zephyr target, drop the per-worker
   stack to 512 B by adding (to a per-board fragment that overrides
   the C-level KVB knobs — typically via a board-specific
   `target_compile_definitions(app PRIVATE -DKVB_DEFAULT_STACK_SIZE=512)`
   block in a per-board CMake fragment).

3. **The console UART needs a different routing.**
   Most boards already chose a sensible `zephyr,console`.  When that is
   wrong for your wiring, a `<board>.overlay` redirects it without
   touching the upstream board files.

## Adding a board

1. (Optional) Drop a `<board>.conf` here with the overrides you need.
2. (Optional) Drop a `<board>.overlay` here for devicetree changes.
3. Build with `west build -b <board> .` from the application root.

No source changes required.  The same KVB application sources work
for every Zephyr board.
