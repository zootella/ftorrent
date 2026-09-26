//./src-tauri/src/window.rs

use std::path::PathBuf;
use tauri::{AppHandle, Manager, WebviewUrl, WebviewWindowBuilder};
use crate::paths::Paths;
use crate::settings::{self, WindowRecord};

/*
Making ftorrent's one window, at the size and place the user left it, or somewhere sensible the first time.

The window is made here, in code, rather than declared in tauri.conf.json, for three reasons that arrived together. A window declared in the config is built, with its web view inside, before setup runs, and setup is where a second launch finds the lock held and leaves; two launches a few tens of milliseconds apart both built a WebView2 on the same profile, and the one that left took the shared browser process down with it, leaving the winner with no window it could ever show. Made here, from the Ready event, a leaving launch never has a window at all. Second, a portable copy keeps WebView2's profile beside itself rather than in the host's local application data, and the builder's data_directory is the only place that can be said. And third, the window can be built once at the size and place it will keep, instead of built at a placeholder size and corrected by the page.

ftorrent is single window, and that makes this simpler than it is for an application that opens several. A multi-window application can't remember a position, because every window would come back to one rectangle and stack; it dictates the size and lets the operating system pick the spot, and then has to check where the window actually landed, because Windows cascades new windows down a staircase without checking that they fit under the taskbar. ftorrent dictates both, every time, and never lets the operating system place it.

What the settings file records, and when it's replayed. After a session the page has written six numbers, all in CSS pixels: the window's outer position and inner size under [window], and under [screen] the size of the monitor it was on. Beside them is whether the window was maximized; while it is, the page leaves the six numbers alone, so they keep the size and place the window restores to, and the window is built there and then maximized. On startup, the monitor under the middle of the saved window is asked for; the middle rather than the corner, because Windows counts an invisible resize border as part of the window, so a window flush against the left edge of the screen has its corner a few pixels off it. If there is such a monitor and it's the size the file says, the ground beneath the user's preference is the same, and the rectangle is replayed exactly, so a window parked in the top left or against the taskbar comes back there tomorrow, avoiding the chrome while blind to it. Anything else means recalculating: no monitor under the point because an external display was unplugged, or a different size because the user changed the resolution, rotated the monitor, changed the scaling, or plugged into a television. That's the whole guard. It fingerprints the ground rather than reasoning about it, and never enumerates the other monitors.

Recalculating, which is also the first run. The window is sized to a fraction of the primary screen, five eighths wide and half as tall, and dropped at a uniformly random spot inside a field centered on the screen that is three quarters of it each way. The margin the field leaves on every side, an eighth of the screen, is deeper than any taskbar, menu bar, or dock, so the window misses the operating system's chrome by construction without ever asking where the chrome is. The one screen query is the primary monitor's size and scale, needed to state the fractions in CSS pixels, and it carries no judgment. The randomness also keeps an installed copy and a portable copy from stacking exactly when both are on a first run.

One approximation, named. Tauri reports monitors and window positions in physical pixels, and the file holds CSS pixels, so the saved middle is scaled by the primary monitor's scale factor before the monitor under it is asked for. On a single monitor, or several at one scale, that's exact. On monitors with different scales the point can land a little off, and the worst that happens is a fingerprint that doesn't match and a fresh placement, which is the fallback anyway.
*/

//how a first-run window is sized and placed, as fractions of the primary screen: the window is dropped uniformly at random wherever it fits inside the field. These are code rather than settings; a user changes the window by moving it, and the settings file records the result
const FIELD_WIDTH_FRACTION:   f64 = 0.75;  //the field a new window is dropped into: a box centered on the screen, this fraction of its width
const FIELD_HEIGHT_FRACTION:  f64 = 0.75;  //and of its height; the margin left over on each side is what keeps the window off the taskbar, menu bar, and dock without ever asking where they are
const WINDOW_WIDTH_FRACTION:  f64 = 0.625; //the window itself, five eighths of the screen wide
const WINDOW_HEIGHT_FRACTION: f64 = 0.5;   //and half as tall
const FALLBACK_SCREEN: (f64, f64) = (1280.0, 800.0);//a screen to size against when tauri can't name one, so there's still a window; tauri's own default window is 800 by 600

/// Make ftorrent's window, hidden, at the size and place the settings file remembers or a fresh one, maximized if it was; called once, from RunEvent::Ready, and the page shows it once it has drawn
pub fn window_build(app: &AppHandle) {
	let paths = app.state::<Paths>().inner().clone();
	let record = settings::settings_window(&paths.settings);//what the settings file remembers about the window, if anything
	let (x, y, width, height) = record.as_ref().and_then(|record| window_replay(app, record)).unwrap_or_else(|| window_fresh(app));
	let maximized = record.is_some_and(|record| record.maximized);//whatever screen it lands on; the rectangle above is where restoring it goes
	let mut builder = WebviewWindowBuilder::new(app, "main", WebviewUrl::default())//main, because capabilities/default.json grants to that label and lifecycle.rs finds the window by it
		.title("ftorrent")
		.inner_size(width, height)//css pixels, which is what the builder calls logical and what the settings file holds
		.position(x, y)
		.maximized(maximized)
		.visible(false);//the page shows it once it has something to draw, so nothing flashes or jumps
	if !paths.data.is_empty() { builder = builder.data_directory(PathBuf::from(&paths.data)) }//where webview2 keeps its profile, in a subfolder of its own inside the data folder: the host's local application data for an installed copy, exactly where tauri would have put it anyway, and portable/ for a portable one, which is the point
	if let Err(e) = builder.build() { eprintln!("ftorrent could not make its window: {e}") }//a window that never appears is worth a line, though in a release build nobody reads it; the tray still offers exit
}

/// The rectangle the settings file remembers, if one has been recorded and the monitor under its middle is still the size the file says it was; none means recalculate
fn window_replay(app: &AppHandle, record: &WindowRecord) -> Option<(f64, f64, f64, f64)> {
	let recorded = record.width > 0.0 && record.height > 0.0;//zeros are how the file says no size and place has been recorded yet
	if !recorded { return None }
	let primary_scale = app.primary_monitor().ok().flatten().map(|m| m.scale_factor()).unwrap_or(1.0);//the approximation the essay names: css to physical by the primary monitor's scale
	let center_x = record.x + record.width  / 2.0;//the window's middle, not its corner: windows counts a few invisible pixels of resize border as part of the window, so a window flush against the left edge has its corner just off the screen, while the middle of any window a user could see is on a monitor
	let center_y = record.y + record.height / 2.0;
	let monitor = app.monitor_from_point(center_x * primary_scale, center_y * primary_scale).ok().flatten()?;//no monitor there: the ground is gone
	let scale = monitor.scale_factor();
	let width  = (monitor.size().width  as f64 / scale).round();
	let height = (monitor.size().height as f64 / scale).round();
	if width != record.screen_width || height != record.screen_height { return None }//a different screen than the one the position was recorded on, so the position means nothing. Comparing floats exactly is sound here because both sides are whole numbers, rounded the same way: window.js records them with Math.round
	Some((record.x, record.y, record.width, record.height))
}

/// A fresh size and place: a fraction of the primary screen, dropped at random inside the centered field
fn window_fresh(app: &AppHandle) -> (f64, f64, f64, f64) {
	let (origin_x, origin_y, screen_width, screen_height) = match app.primary_monitor() {
		Ok(Some(monitor)) => {
			let scale = monitor.scale_factor();//monitors are measured in physical pixels and the builder wants css ones
			(monitor.position().x as f64 / scale, monitor.position().y as f64 / scale, monitor.size().width as f64 / scale, monitor.size().height as f64 / scale)
		}
		_ => (0.0, 0.0, FALLBACK_SCREEN.0, FALLBACK_SCREEN.1),
	};
	let width  = (screen_width  * WINDOW_WIDTH_FRACTION).round();
	let height = (screen_height * WINDOW_HEIGHT_FRACTION).round();
	let field_width  = screen_width  * FIELD_WIDTH_FRACTION;
	let field_height = screen_height * FIELD_HEIGHT_FRACTION;
	let field_x = origin_x + (screen_width  - field_width)  / 2.0;//the field's top left corner, an eighth of the screen in from the edges
	let field_y = origin_y + (screen_height - field_height) / 2.0;
	let slack_x = (field_width  - width).max(0.0);//how much room the window has to move inside the field
	let slack_y = (field_height - height).max(0.0);
	let x = (field_x + fastrand::f64() * slack_x).round();
	let y = (field_y + fastrand::f64() * slack_y).round();
	(x, y, width, height)
}
