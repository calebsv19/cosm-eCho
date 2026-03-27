# mem_console Demo DB

This directory holds the stable showcase database for `mem_console`.

It intentionally lives outside `build/` so `make clean` does not wipe the demo data.

Default showcase DB:
- `mem_console/demo/demo_mem_console.sqlite`

The `mem_console` binary does not use this file by default anymore.
Use it explicitly with:
- `make -C mem_console run-demo`
- `./mem_console/build/mem_console --db ./mem_console/demo/demo_mem_console.sqlite`

Large-list validation helpers:
- `mem_console/demo/seed_large_list.sh`
- `mem_console/demo/LARGE_LIST_AUDIT.md`

Reset helper:
- `mem_console/demo/reset_demo_db.sh`

Example:
- `./mem_console/demo/reset_demo_db.sh`
