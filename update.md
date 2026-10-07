# Update

The desktop client joins ftorrent.com to check for a newer version and, on the user's click, to install it. The check is built: an installed Mac or Windows copy reads the sidecar beside the file it updates from, on a button press and on its own a day or two apart. The install is built on the Mac: the click downloads `ftorrent.app.zip` into the data folder, checks its SHA-256 against the sidecar's, swaps it in for the bundle in `/Applications`, and starts the new copy while the old one saves and quits. The comments in `desktop/src/stores/update.js`, `desktop/src-tauri/src/net.rs`, and `desktop/src-tauri/src/update.rs` explain it. This note shrinks as the rest gets built.

## Windows

The Windows half of `update.rs` returns an error today, and the page doesn't offer the click there. On Windows the update is `ftorrent.exe` itself: the page downloads it into the data folder the same way, and `update_replace` starts it, detached and with no console, from that file rather than a temporary folder, and never through `cmd` or `powershell`. Setup then asks this copy to exit through its pipe, writes over the install folder, and starts the new copy. A letter carries this to the Windows session once the Mac half has passed its test.

## Writing to /Applications

A standard user, without an administrator's rights, can't write to `/Applications`, so the swap fails there, and only the log says why. The click should only be offered to a copy that can replace itself.

## Failed checks

A check or an install that fails says why in the log and nowhere on the page: the button simply works again.

## The public record

Names and Numbers still calls the update check planned, says the client checks HTTPS against the system's trust roots, and doesn't list the request's headers, `User-Agent: ureq/3.4.2`, `Accept: */*`, and `Accept-Encoding: gzip`, the update's download, or that the file's name, `.app.zip.json` or `.exe.json`, tells the server the platform. The planning doc's Automatic Update section still describes Tauri's updater, per-platform manifests, and counting installs from access logs, and its Security section says nothing the page can call starts a process, where `update.rs` now starts `ditto` and `open` on the page's behalf, though only with the downloaded file and paths worked out in Rust. All of these get corrected to match the code.

## Untested

A `next` edited by hand, sleep and wake, and a few hours hidden near the clock on each platform with the hourly look still firing. On the Mac, how long the new copy waits for the lock, which the log now says, where the first test showed only that it took the lock 0.4 seconds after the old one quit; an install that fails at the swap or the restart; and a standard user's click.
