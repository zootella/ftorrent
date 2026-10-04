# Windows: start at login, and smoke test the uninstaller

A letter for the Windows session. It's public and committed, so it names nobody: in anything written back, say "the user." Git is read-only on both boxes, and the user makes every commit; report results exactly as they came out, failures included; never regenerate a lockfile. A reply goes in a fresh `win2mac.md`.

## Start at login

The Mac session built start at login for both platforms and finished the Mac half against the real machine; the Windows half is written but has never compiled or run. The task here is to make it work on Windows, to the design below, which the user settled after several rounds and which shouldn't change without asking them.

**The design.** One setting, `[login] start` in `ftorrent.toml`, factory `false`, is the user's answer, and the radio on the Options page shows it; only a click there changes it, so the radio never snaps back. A click saves the answer, reads the system, writes ftorrent's entry for a yes only if there's none, removes it for a no only if there is one, and reads again. That's the only time ftorrent writes to the system. At startup and whenever the window gets focus, ftorrent only reads, and when the system differs from the answer, either way, a link shows under the question, **Confirm in Startup Apps**, opening `ms-settings:startupapps`; it goes away once the two agree. ftorrent never puts back an entry it finds missing, since no read can tell a user's removal from anything else. A launch the system makes at sign-in starts hidden, with only the tray icon. The question isn't shown on Linux, and is grayed for a copy outside `%LOCALAPPDATA%\ftorrent`.

**What's written for Windows.** `desktop/src/login.js` has the essay and the Windows half: the entry is a value named `ftorrent` under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, holding `"…\ftorrent.exe" --login`. ftorrent reads, never writes, the user's switch under `HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run`, value `ftorrent`, binary, treating an odd first byte as switched off; Microsoft doesn't document the format, so check that against Task Manager and Settings. `desktop/src/stores/login.js` is the flow. In Rust, `registry.rs` gained `registry_get_binary`; `login.rs` sees `--login` on the command line at setup and reports it through `login_launch`; `instance.rs` keeps `--login` out of the page's arrivals and drops a handoff carrying only it. `win-setup/registry.js` now also removes the `StartupApproved` value at uninstall, beside the Run value it already listed. `capabilities/default.json` allows `ms-settings:startupapps`.

**Accepted for now.** Uninstall removes the Run value but leaves `ftorrent.toml`, so a reinstall finds the answer yes with no entry, and the link points at Startup Apps, which can't add one; No then Yes in ftorrent fixes it. The user knows and has accepted it.

**Check, and report each:**

1. `pnpm compile` builds, the Windows-only Rust above included.
2. Installed copy, Options: the question shows, at No, with no link.
3. Yes: the Run value appears with the command above, and the link doesn't flash up during the click. Turn ftorrent off in Task Manager's Startup apps, click back into ftorrent: the link shows. Click it: Settings opens at Apps, Startup. Turn ftorrent on there, come back: the link is gone.
4. No: the Run value is gone, the `StartupApproved` value is left as Windows wrote it, no link.
5. Quicker than signing out, from a terminal: quit ftorrent and start the installed `ftorrent.exe --login`; it should run with only the tray icon, and nothing about `--login` should reach the main page's report. Then, with that copy running, start `ftorrent.exe --login` again: nothing at all should happen, no window and no entry in the report.
6. Yes, then sign out and back in: ftorrent is running with only the tray icon and no window; the tray's Show brings the window where it was, maximized if it was. Launching ftorrent again from Start does the same.
7. A copy run from anywhere else: the question is grayed and the registry untouched.
8. The uninstall test below, with start at login on, also takes the Run and `StartupApproved` values.

ftorrent keeps a log of each run in `ftorrent-logs` in the home folder when `[log] record = true`; every `login:` line there says what the system answered and what ftorrent wrote, which is quicker than reading the registry by hand.

## The uninstaller

The Windows installer is now ftorrent's own program in `desktop/win-setup`, in place of NSIS, and it is its own uninstaller. Installing has been tried on the real machine; uninstalling hasn't. `desktop/win-setup/README.md` tells the uninstall as six steps under Uninstalling, and this test walks them against the machine.

### Before

1. `pnpm installer` in `desktop`, then install from the reveal, as a user would.
2. Run the installed copy. Answer Yes in the bar about opening torrents, so the fallbacks are written as well as the offer; pin ftorrent to the taskbar and to Start.
3. Turn start at login on. Export the registry entries the uninstall list touches, for a before-and-after: `HKCU\Software\Classes\ftorrent.torrent`, `ftorrent.ftorrent`, `ftorrent.url.magnet`, `ftorrent.url.ftorrent`, `.torrent`, `.ftorrent`, `magnet`, `ftorrent`, `Applications\ftorrent.exe`; `HKCU\Software\ftorrent`; the `ftorrent` value under `HKCU\Software\RegisteredApplications`; the `ftorrent` values under `Run` and `StartupApproved\Run`; and `HKCU\Software\Microsoft\Windows\CurrentVersion\Uninstall\ftorrent`.
4. Leave ftorrent running, with its window hidden in the tray, so step 3 of the uninstall, asking the running copy to exit through its pipe, is part of the test.

### Uninstall and check

Uninstall from Settings, Apps, the way a user would. It should ask nothing. Then check, and report each:

- No `ftorrent.exe` or `ftorrent-engine` process is left, and no ghost icon in the tray.
- `%LOCALAPPDATA%\ftorrent` is gone, with everything in it; one `ftorrent-uninstall-*.exe` in Temp is expected and stays.
- ftorrent is gone from Settings, Apps; the Start menu shortcut and both pins are gone.
- Every key and value from the export is gone, except where another program holds it: `.torrent`'s own key may stay if another client listed itself in `OpenWithProgids`, and `Classes\magnet` stays only if its command no longer runs ftorrent.
- `%LOCALAPPDATA%\com.ftorrent.ftorrent` is still there, with `ftorrent.toml` in it, and downloads are untouched.
- Double-clicking a `.torrent` and clicking a magnet link no longer reach ftorrent.

Then install again, and confirm the Settings page shows the association answer as it was, read from the `ftorrent.toml` the uninstall left.
