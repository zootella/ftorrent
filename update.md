# Update

The desktop client joins ftorrent.com to check for a newer version. The check is built: a Mac or Windows copy reads the sidecar beside its installer, on a button press and on its own a day or two apart, and the comments in `desktop/src/stores/update.js` and `desktop/src-tauri/src/net.rs` explain it. Acting on a newer version, with a single click on the Mac and Windows, comes next. This note shrinks as that gets built.

## Failed checks

When a check fails, the settings page shows ureq's raw error text, like `io: failed to lookup address information`, until we choose better words.

## The public record

Names and Numbers still calls the update check planned, says the client checks HTTPS against the system's trust roots, and doesn't list the request's headers, `User-Agent: ureq/3.4.2`, `Accept: */*`, and `Accept-Encoding: gzip`, or that the file's name, `.dmg.json` or `.exe.json`, tells the server the platform. The planning doc's Automatic Update section still describes per-platform manifests and counting installs from access logs. Both get corrected to match the code.

## Untested

The update check against a newer sidecar, a `next` edited by hand, sleep and wake, and a few hours hidden near the clock on each platform with the hourly look still firing.
