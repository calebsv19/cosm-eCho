# eCho Memory-Check Audit

Repository/source identifier: `mem_console`.

## Scope

`make memory-check-audit` is an opt-in fisiCs memory-check audit for the
graph sort and graph compaction allocation paths. It does not run as part of
the default build or app packaging flow.

## Command

```sh
make -C mem_console memory-check-audit
```

The target:
- builds the app with `BUILD_TOOLCHAIN=fisics` and `--overlay=memory-check`
- links the fisiCs memory-check runtime for that overlay build
- builds a focused audit harness under `build/memory_check/`
- writes captured output to:
  - `build/memory_check/mem_console.stdout`
  - `build/memory_check/mem_console.stderr`

## Latest Result

Last checked: 2026-06-07.

```text
[fisics:memory-check] summary: active=0 leaked_bytes=0 allocs=12 frees=12 double_free=0 unknown_free=0 tracker_failures=0
```

The current audit result is clean.
