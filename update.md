# Update

The desktop client joins ftorrent.com to check for a newer version. The check is built: a Mac or Windows copy reads the sidecar beside its installer, on a button press and on its own a day or two apart, and the comments in `desktop/src/stores/update.js` and `desktop/src-tauri/src/net.rs` explain it. Acting on a newer version, with a single click on the Mac and Windows, comes next. This note shrinks as that gets built.

## A certificate for the Mac

The Mac build is signed ad hoc, `signingIdentity: "-"` in `tauri.conf.json`, so each build's identity is the hash of its own code. macOS records the user's privacy grants, like access to the Downloads folder, against that identity, and so may treat every update as a program it has never met. We measure this first: two builds, a grant given to the first, and whether the second keeps it, and whether the login item does.

If it doesn't, we make a self-signed code signing certificate once, in Keychain on the Mac that builds, and name it as the signing identity. Gatekeeper treats it the same as ad hoc, with no Apple account and no notarization, but the identity becomes the certificate, which stays the same from build to build, so grants carry across updates. The certificate and its private key are state on that Mac rather than in the repository: a build from another Mac, with the certificate exported and imported, keeps the identity, and a build signed by a new certificate costs users one round of prompts. An essay beside the build code gives the commands and says where that state lives.

## Failed checks

When a check fails, the settings page shows ureq's raw error text, like `io: failed to lookup address information`, until we choose better words.

## The public record

Names and Numbers still calls the update check planned, says the client checks HTTPS against the system's trust roots, and doesn't list the request's headers, `User-Agent: ureq/3.4.2`, `Accept: */*`, and `Accept-Encoding: gzip`, or that the file's name, `.dmg.json` or `.exe.json`, tells the server the platform. The planning doc's Automatic Update section still describes per-platform manifests and counting installs from access logs. Both get corrected to match the code.

## Untested

The update check against a newer sidecar, a `next` edited by hand, sleep and wake, and a few hours hidden near the clock on each platform with the hourly look still firing.
