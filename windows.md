# Windows: the uninstaller cleans up the registry

A note for the Windows session. It's public and committed, so it names nobody: in anything written back here, say "the user." Git is read-only on both boxes, the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile.

## What's asked

Build the uninstall cleanup the planning doc describes under Uninstall: an NSIS hook, named in `bundle.windows.nsis.installerHooks` in `desktop/src-tauri/tauri.conf.json`, that removes the registry entries ftorrent writes. Only Windows can build and run the installer, so write the hook there, at `desktop/src-tauri/windows/hooks.nsh`, which is where Tauri's own docs put it. The firewall exemption the same paragraph mentions isn't built yet, so it's out of scope here.

The installer artwork stays NSIS's stock look on purpose; no header or sidebar images.

## What ftorrent writes

`desktop/src/associate.js` is the record, all under `HKCU`, and only from a copy in `%LOCALAPPDATA%\ftorrent`:

- `Software\Classes\ftorrent.torrent` and `Software\Classes\ftorrent.ftorrent`, the file ProgIDs, whole keys
- `Software\Classes\ftorrent.url.magnet` and `Software\Classes\ftorrent.url.ftorrent`, the link ProgIDs, whole keys
- the value `ftorrent.torrent` under `Software\Classes\.torrent\OpenWithProgids`, and `ftorrent.ftorrent` under `Software\Classes\.ftorrent\OpenWithProgids`; values only, since other programs share these keys, though `.ftorrent` can go with `/ifempty` once its value is out
- `Software\Classes\Applications\ftorrent.exe`, whole key
- `Software\ftorrent\Capabilities`, whole key
- the value `ftorrent` under `Software\RegisteredApplications`
- the shared scheme classes `Software\Classes\magnet` and `Software\Classes\ftorrent`, but only while `shell\open\command` still reads `"$INSTDIR\ftorrent.exe" "%1"`, the same test Tauri's template uses for deep links; a class another client took over stays

Then one `SHChangeNotify(SHCNE_ASSOCCHANGED)`, the call `registry_notify` makes, so Explorer drops what it cached.

## Three things found in Tauri's template

We read `installer.nsi` at `tauri-cli-v2.11.4`, the CLI this workspace pins.

- **Hooks run during updates too.** The uninstall section inserts the hooks unconditionally, and the updater runs it with `/UPDATE`. Tauri guards its own shortcut and autostart removal with `${If} $UpdateMode <> 1`, and ours needs the same guard, or every update erases the associations until the next launch writes them back.
- **`Software\ftorrent` is shared with Tauri.** The template records the install location at `Software\${MANUFACTURER}\${PRODUCTNAME}`, and our publisher is `ftorrent`, so that's `Software\ftorrent\ftorrent`, a sibling of our `Capabilities`. Delete `Capabilities` alone, then `Software\ftorrent` with `/ifempty`, never the parent whole.
- **Pre or post.** The planning doc names `NSIS_HOOK_PREUNINSTALL`, but that hook runs before `CheckIfAppIsRunning`: a user who declines to close a running ftorrent aborts the uninstall with the keys already gone. `NSIS_HOOK_POSTUNINSTALL` runs once the files are removed, with `$INSTDIR` and `$UpdateMode` still set, which reads as the better home. Use post unless testing shows a reason not to, and say which in the report so the planning doc is corrected to match.

Tauri already removes a `Run` value named `ftorrent` when not updating, which covers start at login once it's built, and it offers a "delete application data" checkbox, unchecked, that removes `%LOCALAPPDATA%\com.ftorrent.ftorrent`, ftorrent's settings and state. Both stay as Tauri has them.

## How to check it

1. Install, launch once so `associate.js` registers, and export the keys above as the before picture, including `magnet` and `ftorrent` under `Software\Classes`.
2. Uninstall from Add or Remove Programs and confirm every key and value above is gone, that `Software\ftorrent\ftorrent` is still there unless the delete-application-data box was ticked (Tauri removes it only then), and that `.torrent\OpenWithProgids` still holds any other program's values.
3. Set `Software\Classes\magnet\shell\open\command` to some other program's command, install, launch, uninstall, and confirm that `magnet` class survives untouched.
4. Install, launch, then run an update over it (or the installer with `/UPDATE`), and confirm the associations are still there before ftorrent is launched again.
