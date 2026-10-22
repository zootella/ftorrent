# macOS version

_A working note on the macOS versions ftorrent requires and is developed on, decided 2026-10-21._

## The versions

| Version | Name | Released |
| --- | --- | --- |
| 11 | Big Sur | 2020-Nov-12 |
| 12 | Monterey | 2021-Oct-25 |
| 13 | Ventura | 2022-Oct-24 |
| 14 | Sonoma | 2023-Sep-26 |
| 15 | Sequoia | 2024-Sep-16 |
| 26 | Tahoe | 2025-Sep-15 |
| 27 | Golden Gate | 2026-Sep-14 |

- Big Sur is the first macOS on Apple silicon, and ftorrent is Apple silicon only, so the table starts there.
- Sonoma came first and Sequoia second, the reverse of alphabetical order.
- After 15, Apple numbered every platform by year: a version released in September carries the number of the following year, so 16 through 25 never existed.

## The decision

- Minimum: macOS 15 Sequoia, set by `minimumSystemVersion` in `desktop/src-tauri/tauri.conf.json`.
- Development: macOS 27 Golden Gate.
- Cadence: the development Mac upgrades every other year, on the odd versions, 29 in 2028 and 31 in 2030.
- Windows: 10 and 11, developed on 10. Windows 7 isn't supported.

The minimum and the development version mean different things. The minimum is a hard stop: Tauri writes it into the app's Info.plist as `LSMinimumSystemVersion`, and macOS refuses to open the app on anything older. It sits at 15 because the engine's libtorrent wheel is built for macOS 15. The development version is the one we code and test on, so it and every later version are known correct. The versions between the two still run the app; they just aren't the ones it was tested on.

Windows and the Mac differ in how people upgrade. A Windows version lasts the better part of a decade, and a school's or company's IT department often decides when it changes. Mac users upgrade quickly: the upgrade is free, System Settings offers it, and the crowd likes the newest thing. TelemetryDeck's [survey](https://telemetrydeck.com/survey/apple/macOS/versions/) had 27 at 13% at the end of September 2026, two weeks after its release.

So we develop on the newest macOS, ahead of where Mac users are going rather than behind them. We skip 26 Tahoe and take the development Mac from 15 straight to 27, through the upgrade System Settings advertises, the same path users take, rather than a downloaded installer for an older version. Upgrading every other year keeps the development Mac on either the current macOS or the one before it.

Each macOS also tightens what an app may do, in privacy settings and the firewall above all, and ftorrent is signed with its own identity rather than one Apple issues, which some of those rules treat differently: local network privacy follows the code signature, which Apple says it tracks reliably only for Apple-issued identities. Developing on the newest shows us a tightening the year it arrives, not a year later.

The cost is that once the development Mac is on 27, it no longer runs 15 or 26, while most Macs are still on 26. A change made after the upgrade could break on those versions without anyone seeing it, so anything that reads parts of macOS that change between versions says which version it was seen on.

## Still open

- Upgrade the development Mac to 27 through Software Update in System Settings.
- Measure the disk image window's title bar on 27. `dmg.js` holds the measurement from Sequoia, where the title bar is 28 points, and Tahoe redrew window chrome.
