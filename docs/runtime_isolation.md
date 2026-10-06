# Runtime isolation and package seeds

The runtime directory owns app preferences and generated runtime resources.
`MEM_CONSOLE_RUNTIME_DIR` selects an absolute runtime root; invalid or unwritable
roots fail without falling back to a shared temporary directory. A package's
runtime namespace selects its default directory. `eCho Main Edit.app` uses
`MemConsole-Main-Edit`; standard eCho uses `MemConsole`.

Startup reads preferences only from the selected runtime. Main Edit and custom
runtime roots do not fall back to standard or legacy preference files. The
standard default runtime retains its legacy `.local/share/mem_console` preference
compatibility. A saved output root cannot override an explicitly selected runtime.
Existing preference files are not moved or deleted.

Database selection preserves `--db`, an explicitly supplied `CODEWORK_MEMDB_PATH`,
and a DB intentionally selected in that runtime's own preferences, in that order.
The launcher distinguishes its generated default DB path from an explicit override
with `MEM_CONSOLE_LAUNCHER_DEFAULT_DB`. Without a selection, the package uses its
runtime seed. An intentional external DB selection remains a reference: no copy,
move, or migration is performed by path resolution. Opening the app normally can
still migrate that database through core_memdb.

Packaging always generates a new empty database using the selected mem_cli/schema
code. Integrity, schema version 6 and zero application records are verified before
exclusive installation into the new bundle. Ignored source `data/default.sqlite`
and the former `PACKAGE_DEFAULT_DB_SRC` override are never copied. An existing
runtime database is preserved by the launcher's first-run seed guard.

Verification: `make run-runtime-isolation-test run-package-empty-seed-test` runs
synthetic preference and database fixtures. The tests cover canonical/legacy
preference isolation, saved and explicit DB choices, invalid roots, ambient seed
exclusion, destination preservation and rejection of populated seeds. Both targets
are part of `make test`. No real user database is required.

Before a separately authorized Desktop installation, preserve the old bundle,
app preferences (`mem_console.app.pack`), each selected database and its `.ui.pack`
sidecar. Quiesce writers or use SQLite's consistent backup API so WAL contents are
included; verify restore on copies. Keep original data until rollback is no longer
needed. This change does not alter schema 6, UI prefs 11 or app prefs 2, and does
not install or activate any worker or service.

Release packaging accepts a job root under the checkout's or CodeWorkData's
`mem_console/build/release-authenticated` directory, including a bound
`job/targets/target` path. The destination must be absent; traversal, symlink
ancestors and occupied outputs fail before packaging. Signed bundles, archives
and launcher diagnostics remain under that selected root. `CODEWORK_DATA_ROOT`
may select the configured data parent; its default is `~/CodeWorkData`.
`python3 tests/test_release_root.py` proves containment and make-path routing
using synthetic directories. These rules do not install the generated app.
