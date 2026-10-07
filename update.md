# Update

The desktop client checks ftorrent.com for a newer version, and on the user's click it installs it. The check is built and sound: an installed Mac or Windows copy reads the sidecar beside the file it updates from, on a button press and on its own a day or two apart. The install is built on the Mac and has worked end to end, but its design is brittle at the core, and it's back on the drawing board. This note says why, what the established updater on the Mac does instead, and where the redesign is headed.

## What's built

The click reads the sidecar again, checks that the data folder's volume has 1 GiB free, downloads `ftorrent.app.zip` into a temporary folder, `update` in the data folder, checks its SHA-256 against the sidecar's, and unzips it there. Then the running copy moves its own bundle into the temporary folder, moves the newer version into `/Applications`, removes the temporary folder, starts the new copy by its path with `--update`, and quits. The new copy waits up to 15 seconds for the lock rather than handing over to the old one. `desktop/src/update.js` holds the sequence, and its essay lists every path; Rust offers only general commands beneath it. A check or an install that fails says why in the log and nowhere on the page, and the button simply works again.

The preparation is solid: nothing touches the installed app until the space is checked, the download is whole, its hash matches, and the zip has unpacked. The trouble is everything after.

## Why it's brittle

- **Moving a running app's bundle breaks its privacy grants for the rest of its life.** Once `/Applications/ftorrent.app` is moved, macOS's privacy daemon can no longer find the code of the process still running from it, and reads and writes in Downloads, Documents, and Desktop fail with "Operation not permitted" until the app is relaunched. The old copy quits right after the switch, and quitting stops the engine, which is when libtorrent will save resume data into the download folders, usually under `~/Downloads`. Those writes would fail silently, and every torrent would re-check on the next launch. It's invisible today only because there are no torrents yet.
- **A slow quit can strand the user.** The new copy gives up on the lock after 15 seconds. An engine saving resume data for many torrents could take longer, and then the new copy exits, the old one finishes quitting, and nothing is running.
- **Finder can keep a stale view of the app.** After the first version of this update swapped bundles, Finder held a reference to the departed bundle for at least an hour, and dragging a fresh copy from the disk image onto Applications failed with "an item with the name "" already exists" (error -48), until Finder caught up on its own. The current code doesn't tell the system the bundle changed.
- **If starting the new copy fails after the switch, the old one runs on from a moved bundle**, its grants broken for the rest of the session.
- **A home folder on another drive makes the running app's own bundle cross volumes**, copied out and then removed while it runs, which is where the code's partial-copy guard and its exceptions for keeping the temporary folder came from.
- **A standard user, without an administrator's rights, can't write to `/Applications`**, so the switch fails after the whole download.
- **This version's sequence has never run.** It moved from Rust into the page after the first test and was reviewed but not exercised.

Three of these share one root: the old process keeps working after its bundle has moved. Having the old copy finish its protected work before the switch would narrow it, but that's a promise every later feature would have to keep, and one new write after the switch would break it again, silently.

## What Sparkle does

Sparkle is the update framework most Mac apps outside the App Store use, and its source answers each of these:

- **A separate installer does the replacing.** Sparkle ships a small tool, Autoupdate, and submits it to launchd as a one-time job in the user's session, so it belongs to launchd rather than to the app and outlives it. Its source says the deprecated `SMJobSubmit` is "the only way to submit a non-permanent helper," and it needs no authorization in the user's own domain. A second small agent shows progress, asks the app to quit, and relaunches it; that one is copied out of the bundle first, since it depends on files inside the bundle being replaced.
- **The app quits before its bundle is touched.** The agent sends an ordinary quit, `NSRunningApplication terminate`, which "gives the application or user a chance to delay or cancel," and the installer waits on the kernel, a kqueue `NOTE_EXIT` watch on the process, with a check right after the watch is set for an app that already exited and a backup check two seconds later.
- **The new bundle is staged on the app's own volume.** When the old and new bundles are on different volumes, Sparkle first moves the new one into a temporary folder on the destination's volume.
- **The switch is one atomic swap**, `renamex_np` with `RENAME_SWAP`, falling back to moving the old bundle aside, moving the new one in, and restoring the old one on failure.
- **The new bundle is touched afterward**, its times updated with `futimes`, under the comment "Register the new bundle with LaunchServices and the system."
- **The app is relaunched by its path**, so the system opens that exact copy rather than whichever one shares its identifier.

## Where it's headed

On the Mac, ftorrent itself becomes the orchestrator, with no second program. The newer version, unzipped in the temporary folder, is started in an installer mode that skips the window and the engine: it waits for the old copy to quit through its ordinary quit, with its bundle untouched and every grant intact, then swaps the bundles, touches the new one, opens `/Applications/ftorrent.app` by its path, and exits. A slow quit no longer strands anyone, since the installer waits as long as the quit takes. How the installer mode fits the rule in `desktop/src-tauri/src/lib.rs`, that Rust stays general and the page holds the application, is the first question for the design, since the mode runs before any page exists.

On Windows the setup program already is that orchestrator: it asks the running copy to exit through its pipe, waits for its files to come free, writes over the install folder, and starts the new copy. `windows.md` carries that half, and the redesign on the Mac doesn't change its shape.

## The public record

Names and Numbers still calls the update check planned, says the client checks HTTPS against the system's trust roots, and doesn't list the request's headers, `User-Agent: ureq/3.4.2`, `Accept: */*`, and `Accept-Encoding: gzip`, the update's download, or that the file's name, `.app.zip.json` or `.exe.json`, tells the server the platform. The planning doc's Automatic Update section still describes Tauri's updater, per-platform manifests, and counting installs from access logs, and its Security section says nothing the page can call starts a process, where `process.rs` now runs and starts programs for the page, as general commands. All of these get corrected once the design settles.

## Untested

A `next` edited by hand, sleep and wake, and a few hours hidden near the clock on each platform with the hourly look still firing. For the redesigned Mac install, the test covers the click to the new window, a slow quit, a home folder on another drive, a standard user's click, and, right after the update, dragging a release from the disk image onto Applications, which has to replace it cleanly.
