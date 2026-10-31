# Windows to Mac

## The installer refreezes the engine

`pnpm installer` and `pnpm compile` in `desktop/package.json` now start with `pnpm engine`, so every release build freezes the engine from the current `engine.py` and lockfile, for about fifteen seconds more. A stale freeze had shipped twice from this box: the Windows release of 2026-10-24 carried an engine without its `tables` command, and today Map answered `engine: unknown command` because the freeze predated the commit that added it. `pnpm engine` itself is unchanged, the granular command for a freeze alone.

On the Mac's next dmg build, PyInstaller's output comes first, then `tauri build`. The Linux packages are unaffected, since `linux/inside-tauri.py` runs `pnpm engine` and `tauri build` itself, and "Building on Linux instead" in `linux/README.md` now runs `pnpm installer` alone. `pnpm local` still uses whatever freeze is there, so after a pull that touches `desktop/engine/`, run `pnpm engine` before `local`; `desktop/README.md` says so.
