# eCho Main Edit Worktree

Last updated: 2026-08-27

The persistent Main Edit lane is the normal implementation workspace for eCho.
It gives active source work a stable checkout and a separately identifiable
package without changing the canonical desktop app.

## Fixed Identity

- canonical checkout: `<CodeWork>/mem_console`
- persistent worktree: `<CodeWork>/_worktrees/mem_console_main_edit`
- branch: `codex/mem-console-main-edit`
- app: `eCho Main Edit.app`
- bundle identifier: `com.cosm.echo.main-edit`
- runtime and log namespace: `MemConsole-Main-Edit`
- package profile: `main-edit`
- package output: `build/targets/<target-triple>/dist/dev/main-edit/eCho Main Edit.app`

The canonical `eCho.app`, `com.cosm.echo` bundle identity, `MemConsole`
runtime/log roots, and Desktop destination remain separate.

## Normal Flow

1. Freshly read both lane identities, cleanliness, worktree inventory, and
   ahead/behind counts.
2. Confirm that no process owns the source worktree or the Main Edit app.
3. Work only in the persistent Main Edit worktree. If concurrent features are
   unavoidable, serialize overlapping files or use a short-lived branch from
   this lane without creating another standing program worktree.
4. Run `git diff --check`, focused tests, and the program verification ladder.
5. Build and verify the isolated package:

   ```sh
   make -C <CodeWork>/_worktrees/mem_console_main_edit package-desktop-main-edit-self-test
   ```

6. Commit a coherent checkpoint on `codex/mem-console-main-edit`.
7. Rebuild the Main Edit package from the clean committed source so its embedded
   identity names the exact commit.
8. Adopt into canonical only when canonical is clean, contains no unique
   commits, and a fast-forward is possible. Re-run canonical verification after
   adoption.

## Package Safety Gates

`package-desktop-main-edit` fingerprints tracked source plus live worktree state
before and after packaging. If source changes during the build, it discards the
generated app and fails. The package embeds `build_identity.json`, including
the source commit/state, binary digest, target architecture, toolchain, profile,
version, and build label.

`package-desktop-main-edit-self-test` verifies the plist identity, embedded
identity, binary digest, isolated runtime/log configuration, and code signature.
It does not refresh or open either Desktop app.

`package-desktop-main-edit-refresh` is a separate, explicit operator action. It
audits the distinct Main Edit app process before replacing only
`~/Desktop/eCho Main Edit.app`; it never replaces `~/Desktop/eCho.app`.

## Boundaries

- Do not edit `VERSION`, publish, deploy, push, or mutate release/Registry state
  as part of ordinary Main Edit work.
- Do not reset, clean away, or repurpose unrelated worktrees or ignored local
  state.
- A successful local package is development evidence, not a release artifact.
- Keep source adoption, Desktop refresh, release packaging, and publication as
  distinct authority boundaries.
