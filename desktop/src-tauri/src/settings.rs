//./src-tauri/src/settings.rs

use serde::Deserialize;

/*
The early half of ftorrent's settings, and the only part of them Rust knows about.

The page owns ftorrent.toml. settings.js has the schema, the factory values, the repair of a bad file, the comments it writes back, and the whole round trip: the page reads the file at startup, keeps the settings in its store, writes the file when a setting changes, and hands the text to desktop.rs to be written once more when ftorrent exits. None of that lives here. What this module adds is a second, earlier, much smaller read, for the one question that has to be answered before the page exists: how big to make the window, and where.

That question can't wait for the page because the window is built before any JavaScript has run. So this parses ftorrent.toml, takes the window's six numbers and its maximized flag out of [window] and [screen], and ignores everything else in the file. It reads and never writes; writing stays with the page, and keeping it that way is what stops there being two things that believe they own this file. And it is a partial reader on purpose: it doesn't know the schema, doesn't repair anything, doesn't supply factory values for the rest, and doesn't complain about a file it can't make sense of. It answers "nothing recorded" and window.rs picks a fresh size and place instead, which is what a first launch does anyway. A malformed file is still the page's to report, a moment later, when it reads the same file properly.

Every number is in CSS pixels, the one unit that means the same thing on every screen: the page records them that way, this reads them that way, and the window builder takes them that way. While the window is maximized the page records only that, and leaves the numbers where the window was before, so they're always a size and place to restore to.
*/

#[derive(Deserialize)]
struct SettingsFile { window: Option<WindowSection>, screen: Option<ScreenSection> }//only the two sections this reads; serde passes over every other key in the file, since nothing here asks it to deny unknown fields

#[derive(Deserialize)]
struct WindowSection { x: Option<f64>, y: Option<f64>, width: Option<f64>, height: Option<f64>, maximized: Option<bool> }

#[derive(Deserialize)]
struct ScreenSection { width: Option<f64>, height: Option<f64> }

/// Where the window was and how big, the size of the screen it was on, and whether it was maximized, as the page last recorded them
pub struct WindowRecord {
	pub x: f64,//the outer position of the window's top left corner, in css pixels
	pub y: f64,
	pub width: f64,//and its inner size; all zeros until the page has recorded a first place
	pub height: f64,
	pub screen_width: f64,//the size of the monitor it was on, the fingerprint window.rs checks before replaying the four numbers above
	pub screen_height: f64,
	pub maximized: bool,//open maximized, over the four numbers above, which are where the window goes when restored
}

/// The window the settings file remembers, or none if the file is missing or unreadable
pub fn settings_window(path: &str) -> Option<WindowRecord> {
	let text = std::fs::read_to_string(path).ok()?;//no file yet on a first launch, or no path at all without a data folder, neither of which is worth a line anywhere
	let file: SettingsFile = toml::from_str(&text).ok()?;//anything this can't parse is the page's to report, once it's up and reads the same file properly
	let window = file.window?;
	let screen = file.screen?;
	Some(WindowRecord {
		x: window.x?, y: window.y?, width: window.width?, height: window.height?,
		screen_width: screen.width?, screen_height: screen.height?,
		maximized: window.maximized.unwrap_or(false),//a file written before this key existed opens normal
	})
}
