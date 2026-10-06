# Update and Wave

Two small features join the desktop client to ftorrent.com. The update check is built: a Mac or Windows copy reads the sidecar beside its installer, on a button press and on its own a day or two apart, and the comments in `desktop/src/stores/update.js` and `desktop/src-tauri/src/net.rs` explain it. The wave is still a design: a button the user presses to say hello, counted on ftorrent.com's home page as the number of waves in the last 30 days, rolling. This note shrinks as the wave gets built, and its last section grows into Wave Hello, a user guide on docs.ftorrent.com.

## Failed checks

When a check fails, the settings page shows ureq's raw error text, like `io: failed to lookup address information`, until we choose better words.

## The wave button

A button the user presses, which sends one `POST` to `https://ftorrent.com/wave`. The client sends nothing on its own, retries nothing in the background, and needs no setting. Where the button lives, and every word on and around it, is still to be chosen. So is whether the request carries anything about the client, its version and platform, beyond the wave itself.

## Counting waves

The server counts each wave and serves the count of the last 30 days, rolling. It keeps only counts per day, with no addresses and nothing about the sender, so the 30-day number is the sum of the last 30 days. The count is of waves, not people: a user who presses twice is counted twice, and the home page says so.

The count has to hold up against a friendly attacker: someone who likes the project, clones its open source, points Claude Code at it, and sets a gaming rig with a powerful GPU to running the number up overnight. Everything in the client is public, so no secret in the client can be the defense, and the only cost we can impose is the attacker's own compute. The first version ships with that defense in place; a plain counter that trusts every request isn't a first step toward it.

The shape we're researching is a proof of work. The client spends seconds of its own CPU minting each wave, the server verifies it in microseconds, so a flood of junk waves costs the server almost nothing, and the puzzle is one a GPU solves little faster than a CPU. Each wave is dated and unique, so one solution can't be posted twice or minted far ahead. Equi-X, the construction Tor ships against onion-service floods, is one candidate; we'll survey the field and measure before choosing. One constraint shapes the choice: the client runs no code it generates at runtime. A just-in-time compiler in a desktop program trips notarization entitlements on the Mac and antivirus heuristics on Windows, and an easter egg isn't worth either alarm. Beside the work, a per-address limit at the proxy caps what one connection can send, without the counter itself storing addresses.

The URL the home page reads and the response format are still to be settled.

## The number on the home page

The site is static, so the home page fetches the count from the server and draws it. Served from ftorrent.com itself, it needs no CORS.

## The public record

Names and Numbers still calls the update check planned, says the client checks HTTPS against the system's trust roots, and doesn't list the request's headers, `User-Agent: ureq/3.4.2`, `Accept: */*`, and `Accept-Encoding: gzip`, or that the file's name, `.dmg.json` or `.exe.json`, tells the server the platform. The planning doc's Automatic Update section still describes per-platform manifests and counting installs from access logs. Both get corrected to match the code once the wave is in, with the wave's own fields listed beside the update check's.

## Untested

The update check against a newer sidecar, a `next` edited by hand, sleep and wake, and a few hours hidden near the clock on each platform with the hourly look still firing. The wave, from a press to the count, and the count letting a wave go after 30 days.

## Wave Hello

ftorrent has no telemetry: no analytics, no crash reporting, no usage pings, nothing counting its users in the background. There's no setting to opt out of telemetry, because we never built any to opt out of. Names and Numbers lists every server the client reaches on its own and every field it sends.

That leaves the project with no idea how many people use it, so we built the wave as a fun alternative to the usual telemetry, which erodes privacy and is hard to opt out of. A user who finds the button and presses it is choosing to be counted, once, on purpose. The home page shows the total honestly. If nobody uses ftorrent, it shows zero. If a few thousand people use it and like what it does and how it's built, a handful of them will wave, and the number will show that.

The wave is a bit of an easter egg. A user who wonders "wave? what's that?" lands here and finds exactly what the button sends, where it goes, what the server keeps, and why we built it this way. Our values are visible in what the software does, not only in what we say about it.
