use tauri::{AppHandle, Manager, Window, WindowEvent};

/*
ftorrent keeps running with its window closed, the way a file transfer application does: closing the window is putting it away, and quitting is a separate, explicit act. There is no setting for this. One process, one window, on every platform: a second launch never opens another window, it hands over to this one (instance.rs), and this module is how that one window goes away and comes back.

Closing hides. The close button on Windows and the red button on macOS hide the window instead of closing it, except from a fullscreen Space on the Mac, as the essay above window_event describes, so the page and its webview stay alive, keep their state, and come back at once; the webview's footprint is small next to the engine's. The window is created once when ftorrent starts and destroyed once when it quits.

Quitting is on each platform's own terms. On macOS it's Quit in the app menu, ⌘Q, or Quit in the Dock icon's menu, all of which macOS provides, and Quit in the menu bar icon's menu, which ftorrent provides. That icon, the mark as a glyph near the clock, is there for as long as ftorrent runs, the way Dropbox's and Docker Desktop's are, and the Dock icon is there only while the window is: closing the window hides it and moves the app to the Accessory activation policy, the policy of an app that lives in the menu bar, which takes the Dock icon, the ⌘-Tab entry, and the app's menu bar away, so a running ftorrent costs the desktop one small glyph and nothing else; and bringing the window back sets the policy to Regular first, so the window returns as a Regular app's, with its Dock icon, its menu bar, and its place in ⌘-Tab. A Dock tile the user keeps stays through all of this, as a launcher without a dot, which the essay above dock explains, and clicking it, or launching ftorrent again from Spotlight or Launchpad, sends Reopen, which brings the window back. On Windows there's no Dock, so a tray icon stands in: clicking it brings the window back, and its menu has Show and Exit; its icon is the mark as a monochrome glyph, white on a dark taskbar and black on a light one, where Windows programs put their full-color icon, so that ftorrent reads as a glyph beside the system's own, which is on purpose; and the window has a File menu with Exit, the way a Windows client's File menu ends, for whoever never looks at the tray. On a fresh Windows the tray icon starts behind the taskbar's overflow chevron, where Windows puts every new icon until the user drags it out, and a program can't promote its own; so the way back a user finds first is launching ftorrent again, from the Start menu or a pinned button, and that works: the second launch runs for a moment, hands over through the pipe (instance.rs), and leaves, and this copy brings its window forward. Every way of quitting reaches the Exit run event in lib.rs, which stops the engine and, on Windows, takes the tray icon down before the process goes.

The words follow each platform too, wherever its users already know one. Windows says Exit, and macOS says Quit. On Windows the window has the menu bar Windows programs have had since the 1990s, File with Exit, Tools with Options, and Help with About, each with an access key and none with a shortcut, and the tray's menu has Show and Exit; on macOS the app menu is the one macOS provides, and the menu bar icon's menu has Show ftorrent and Quit ftorrent, the words of Apple's own menus, where every app menu has Hide ftorrent, Show is its complement, and Quit ftorrent is the app menu's last item. What ftorrent calls its settings follows the same way: Options on Windows, the classic word, which qBittorrent uses too, and Settings everywhere else, the word macOS has used since Ventura. The menu's words are written here, since Rust builds the menu before any page exists, and the page's words for the same things, like the Options page's title, come from settings.js, where the page asks which platform it's on. Past these few words, the page reads the same on every platform.

Focus is taken only when the user asks for the window. bring_forward shows the window, restores it if it's minimized, and gives it focus, and it runs only for a second launch's handoff, the tray or menu bar icon, and the Dock, each of which begins with the user reaching for ftorrent; without the focus, a window brought back by a handoff on Windows can land behind whatever the user was just in. The app never takes focus on its own: at startup the page shows the window and leaves it to the operating system whether a newly launched app comes to the front, which it gets right for the Dock, Finder, Spotlight, and the Start menu, and holds back, on purpose, for a launch it didn't see the user make.

Linux keeps closing as quitting for now. It has no tray here and no handoff yet, so a hidden window would be one nobody could get back. The page listens for the close too, to write the settings, where the window was among them, and in Tauri a page that listens decides whether the close goes ahead. It always declines, so that on macOS and Windows the hide is the only thing a close does. So on Linux, Rust turns the close into a quit, which reaches the same Exit run event, where the text the page handed down for that moment is written.
*/

/// Show the window, restore it if it's minimized, and give it focus; the tray, the Dock, and a second launch all bring ftorrent back this way
pub fn bring_forward(app: &AppHandle) {
	if !app.state::<crate::window::Revealed>().0.load(std::sync::atomic::Ordering::SeqCst) { return }//the page hasn't placed the window yet, and shows it itself in a moment; window.rs says why this waits
	if let Some(window) = app.get_webview_window("main") {
		#[cfg(target_os = "macos")]
		dock(app, true);//the Dock icon first, so the window comes back as a Regular app's, with its menu bar and its place in ⌘-Tab
		let _ = window.show();
		let _ = window.unminimize();
		let _ = window.set_focus();
	}
}

/*
The red button on a fullscreen Mac window, which takes two clicks to put the window away.

A fullscreen window on macOS is a Space of its own, and a hidden window still owns its Space. So hiding straight from fullscreen, the way every other close hides, leaves the Space behind, empty and black under ftorrent's menu bar, with the Dock bringing the window back fullscreen inside it. ⌘H keeps the Space too. Instead, the first click takes the window out of fullscreen: macOS plays its exit animation, collapses the Space, and returns the window to the desktop it came from, at its size and place from before. In a Space, then, red does what green does, and a second click, now on an ordinary window, hides it as always.

One click that does both, the way a Mac app that closes its last window disappears along with its Space, is what we'd rather have, and we tried it. tao clears the window's fullscreen state and then sends a resize once the exit animation finishes, so that resize is the moment to hide. Hidden there, or a turn of the event loop later, the window really does hide, and then AppKit shows it again and makes it key a moment afterward, as it finishes the transition. An app that destroys its window on close never meets this, since there's nothing left to show; ftorrent keeps its window so the page keeps its state. So the second click is the user's: one extra click, and never an empty Space.
*/

/// The close button hides the window rather than closing it, on macOS and Windows, and quits on Linux; on a fullscreen Mac window, it leaves fullscreen first
pub fn window_event(window: &Window, event: &WindowEvent) {
	if let WindowEvent::CloseRequested { api, .. } = event {
		if cfg!(any(target_os = "macos", target_os = "windows")) {
			api.prevent_close();//the window stays, and so does everything behind it
			if cfg!(target_os = "macos") && window.is_fullscreen().unwrap_or(false) {
				let _ = window.set_fullscreen(false);//out of the Space, which macOS then collapses; the next click hides
			} else {
				let _ = window.hide();
				#[cfg(target_os = "macos")]
				dock(window.app_handle(), false);//and the Dock icon goes with it, after the window is already gone, so the Dock never shows an icon with nothing behind it
			}
		} else {
			window.app_handle().exit(0);//linux: the page declined the close, so quitting is said outright; Exit writes the settings and stops the engine
		}
	}
}

/*
The Dock's dot, and the kept tile that goes dark.

The dot under a Dock icon is the Dock's indicator for an open application, in the words of the setting that turns the dots off, Show indicators for open applications, under Desktop & Dock. Open, to the Dock, means running as a foreground app, under the Regular policy: an app with a Dock icon and a menu bar of its own. A background app, under Accessory, never gets a dot, however long it has been running, because to the Dock it isn't open. So a tile the user keeps in the Dock goes dark the moment ftorrent's window closes, while ftorrent runs on, and a user who reads the dot as "running" is misled for a glance. The glyph near the clock is the running indicator; the dot says whether there's a window.

This is the grain of macOS, and every app that lives in the menu bar pays the same price: a kept Docker Desktop tile shows no dot while its dashboard is closed. Transmission does the other thing. Its window closes and its Dock icon stays lit, as every app with a window does, and a user arriving from it is the one most likely to misread the dark tile. No policy gives both, a dot on a kept tile and no icon for an unkept one, so ftorrent takes the pattern of the apps that run in the background, which is what it is while its window is away. The misreading costs nothing past the glance: a click on the dark tile reaches this same process through Reopen rather than starting another, and the dot comes back with the window.
*/

/// On macOS, whether ftorrent is in the Dock. Regular is the activation policy of an app with a window, and Accessory is the policy of an app that lives in the menu bar: no Dock icon, no ⌘-Tab entry, and no menu bar of its own, which is why Quit is in the menu bar icon's menu. Docker Desktop's Dock icon comes and goes with its dashboard window the same way
#[cfg(target_os = "macos")]
fn dock(app: &AppHandle, present: bool) {
	use tauri::ActivationPolicy::{Accessory, Regular};
	if let Err(e) = app.set_activation_policy(if present { Regular } else { Accessory }) { crate::log::log(&format!("the activation policy did not change: {e}")) }//noted and nothing more: the window still hides and shows, with the Dock icon staying however it was
}

/// On Windows, the menu bar a Windows program has had since the 1990s: File with Exit, Tools with Options, and Help with About. The close button hides, so File, Exit is the in-window way to quit, the one that needs no tray icon and that keyboard users reach; µTorrent, qBittorrent, and Deluge all have it. Options and About each open a page, which the page's router does; Rust only says which item was picked. Called once from setup
#[cfg(target_os = "windows")]
pub fn menu_install(app: &AppHandle) -> tauri::Result<()> {
	use tauri::menu::{Menu, MenuItem, Submenu};
	use tauri::Emitter;//for emit, which hands the page an event
	let brand_name = &app.package_info().name;//brandName, the product name from tauri.conf.json, which people read
	let exit = MenuItem::with_id(app, "exit", "E&xit", true, None::<&str>)?;//the same id as the tray's Exit, so both reach one handler. The ampersand makes x its access key, underlined while Alt is held, so Alt, F, X quits, the way a Windows File menu does; and no shortcut, since the system's own Alt+F4 is a close, which now hides
	let file = Submenu::with_items(app, "&File", true, &[&exit])?;//and F the menu's, so Alt+F opens it
	let options = MenuItem::with_id(app, "settings", "&Options...", true, None::<&str>)?;//windows' classic word for settings, where qbittorrent has it too; the page's settings.js names its page Options on windows to match. The id is the name of the route it opens
	let tools = Submenu::with_items(app, "&Tools", true, &[&options])?;
	let about = MenuItem::with_id(app, "about", format!("&About {brand_name}"), true, None::<&str>)?;//the id the name of its route too
	let help = Submenu::with_items(app, "&Help", true, &[&about])?;//last, the way a Windows menu bar ends
	app.set_menu(Menu::with_items(app, &[&file, &tools, &help])?)?;//on windows, a menu set on the app is the menu bar of its window
	app.on_menu_event(|app, event| match event.id().as_ref() {//every menu event reaches this handler and the tray's both, as tauri documents, so each acts only on the ids it knows: the tray's Show falls through here, and exit, in both menus, runs twice, which is harmless, since the second finds nothing left to stop
		"exit" => app.exit(0),//reaches the Exit run event, which stops the engine
		route @ ("settings" | "about") => { let _ = app.emit("menu", route); }//the page opens the route of that name; a menu is only there to click once the page has shown the window, so the page is always listening by then
		_ => {}
	});
	Ok(())
}

/// On Windows, take the tray icon down before the process ends; called from the Exit run event, which every way of quitting reaches. Dropping the icon is what sends the shell its remove message; a process that just exits leaves a ghost icon in the notification area until the mouse touches it
#[cfg(target_os = "windows")]
pub fn tray_remove(app: &AppHandle) {
	let _ = app.remove_tray_by_id("main");//none means it was never built, or is already gone
}

/// The registry key where Windows keeps the user's choice of light or dark, for the taskbar and for apps separately; both Windows 10 and 11 write it there
#[cfg(target_os = "windows")]
const THEME_KEY: &str = r"Software\Microsoft\Windows\CurrentVersion\Themes\Personalize";

/// On Windows, whether the taskbar is light. SystemUsesLightTheme is the taskbar's half of the theme, separate from AppsUseLightTheme, which is what the window follows and what Tauri reports as the theme; Windows 10 ships with the taskbar dark and apps light, so the two differ on most of its machines, and Windows 11 ships with both light. A missing value, which older Windows 10 builds have, means dark, and so does anything but a 1
#[cfg(target_os = "windows")]
fn taskbar_light() -> bool {
	crate::registry::number(THEME_KEY, "SystemUsesLightTheme") == Some(1)
}

/// On Windows, the tray icon for a light or a dark taskbar, at the size the shell draws it. The notification area paints an icon's pixels as they are, so the program carries two icons, the glyph in black for a light taskbar and in white for a dark one, each packed by the icon studio at every taskbar scale, and the layer drawn for this system's scaling is the one handed over, so nothing is resized on the way to the screen; the application icon's first layer, which Tauri would hand the tray by default, is 32 pixels, which the shell shrinks to 16 at 100 percent scaling
#[cfg(target_os = "windows")]
fn tray_icon(light: bool) -> Option<tauri::image::Image<'static>> {
	use windows::Win32::UI::WindowsAndMessaging::{GetSystemMetrics, SM_CXSMICON};
	let wanted = unsafe { GetSystemMetrics(SM_CXSMICON) } as u32;//the small icon size at the system's scaling: 16 pixels at 100 percent, 20 at 125, 24 at 150, 32 at 200, which is the size the notification area draws at and the one LoadIconMetric would choose
	let bytes: &[u8] = if light { include_bytes!("../icons/tray-black.ico") } else { include_bytes!("../icons/tray-white.ico") };//both files are built into the program, so neither can go missing from an installed copy; named as a slice because include_bytes types each file by its length, and the two differ
	let directory = ico::IconDir::read(std::io::Cursor::new(bytes)).ok()?;//the file's directory
	let entry = directory.entries().iter().filter(|entry| entry.width() >= wanted).min_by_key(|entry| entry.width()).or_else(|| directory.entries().iter().max_by_key(|entry| entry.width()))?;//the layer drawn for this size, or the next larger, which the shell shrinks a little; never a smaller one, which it would stretch
	let image = entry.decode().ok()?;
	Some(tauri::image::Image::new_owned(image.rgba_data().to_vec(), image.width(), image.height()))
}

/// On Windows, tell the shell which application this process is, before the tray or the window exists; called once from setup. The identifier is tauri.conf.json's, com.ftorrent.ftorrent, and the Start menu shortcut the installer writes carries the same string as its AppUserModelID. The taskbar matches a running window to a pinned shortcut by that identifier, and a window whose process never said one gets an identity Windows derives from the executable's path instead, so the pin and the running window would be two buttons rather than one, which is why every Electron app makes this same call. Notifications and the jump list key on it too
#[cfg(target_os = "windows")]
pub fn identity_install(app: &AppHandle) {
	use windows::core::PCWSTR;
	use windows::Win32::UI::Shell::SetCurrentProcessExplicitAppUserModelID;
	let wide: Vec<u16> = app.config().identifier.encode_utf16().chain(std::iter::once(0)).collect();
	if let Err(e) = unsafe { SetCurrentProcessExplicitAppUserModelID(PCWSTR(wide.as_ptr())) } { crate::log::log(&format!("the shell did not take the application identifier: {e}")) }//noted and nothing more: the window still opens, under the path-derived identity it had before this call existed
}

/// On Windows, the tray icon that brings the window back and quits; called once from setup
#[cfg(target_os = "windows")]
pub fn tray_install(app: &AppHandle) -> tauri::Result<()> {
	use tauri::menu::{Menu, MenuItem};
	use tauri::tray::{MouseButton, MouseButtonState, TrayIconBuilder, TrayIconEvent};
	let brand_name = &app.package_info().name;//brandName, the product name from tauri.conf.json, which people read
	let show = MenuItem::with_id(app, "show", format!("&Show {brand_name}"), true, None::<&str>)?;//access keys here too, S and x, so the menu works from the keyboard once it's open
	let exit = MenuItem::with_id(app, "exit", "E&xit", true, None::<&str>)?;
	let menu = Menu::with_items(app, &[&show, &exit])?;
	let mut tray = TrayIconBuilder::with_id("main")
		.tooltip(brand_name)
		.menu(&menu)
		.show_menu_on_left_click(false)//left click brings the window back, and the menu is on the right, where Windows users look for it
		.on_menu_event(|app, event| match event.id().as_ref() {
			"show" => bring_forward(app),
			"exit" => app.exit(0),//reaches the Exit run event, which stops the engine
			_ => {}
		})
		.on_tray_icon_event(|tray, event| {
			if let TrayIconEvent::Click { button: MouseButton::Left, button_state: MouseButtonState::Up, .. } = event { bring_forward(tray.app_handle()) }
		});
	if let Some(icon) = tray_icon(taskbar_light()) { tray = tray.icon(icon) }//the glyph in the color the taskbar's theme calls for
	else if let Some(icon) = app.default_window_icon() { tray = tray.icon(icon.clone()) }//the application icon, only if the file built into the program somehow won't read
	tray.build(app)?;//the app keeps it, by its id, for as long as ftorrent runs
	let handle = app.clone();
	crate::registry::watch(THEME_KEY, move || {//the user changed the theme in Settings while ftorrent runs: read it again and show the other icon. The key holds the accent color and more besides, so a change there that isn't the taskbar's theme sets the same icon again, which the shell shows without a flicker
		if let (Some(tray), Some(icon)) = (handle.tray_by_id("main"), tray_icon(taskbar_light())) { let _ = tray.set_icon(Some(icon)); }//set_icon runs on the main thread whichever thread asks, which this one isn't
	});
	Ok(())
}

/// On macOS, the menu bar icon that brings the window back and quits; called once from setup. It stands near the clock for as long as ftorrent runs, and it's the way back once the window is hidden and the Dock icon has gone with it, which is also why Quit is here: under the Accessory policy the app has no menu bar for ⌘Q to reach
#[cfg(target_os = "macos")]
pub fn tray_install(app: &AppHandle) -> tauri::Result<()> {
	use tauri::menu::{Menu, MenuItem};
	use tauri::tray::TrayIconBuilder;
	let brand_name = &app.package_info().name;//brandName, the product name from tauri.conf.json, which people read
	let show = MenuItem::with_id(app, "show", format!("Show {brand_name}"), true, None::<&str>)?;//Apple's word: every app menu has Hide ftorrent, and Show is its complement throughout the system's menus; no ellipsis, since nothing more is asked
	let quit = MenuItem::with_id(app, "exit", format!("Quit {brand_name}"), true, None::<&str>)?;//the app menu's own last item, word for word; the id names the call, as the windows menus' does
	let menu = Menu::with_items(app, &[&show, &quit])?;
	TrayIconBuilder::with_id("main")
		.tooltip(brand_name)
		.icon(tauri::include_image!("icons/tray-template.png"))//the glyph on an 18 point square at Retina, 36 pixels, decoded as the program is compiled; the path is from the crate's folder, where include_bytes! above counts from this file. tray-icon draws a status item's image 18 points tall whatever its size, so the file is drawn at exactly that and nothing is resized on the way to the menu bar
		.icon_as_template(true)//macOS reads the shape alone, the opaque pixels, and paints it in the menu bar's own color: black on a light bar, white on a dark one, and inverted while the menu is open
		.menu(&menu)//a click, left or right, opens the menu, where a mac user expects a menu bar icon's click to go; the way back to the window is its first item
		.on_menu_event(|app, event| match event.id().as_ref() {
			"show" => bring_forward(app),
			"exit" => app.exit(0),//reaches the Exit run event, which stops the engine
			_ => {}
		})
		.build(app)?;//the app keeps it, by its id, for as long as ftorrent runs, and it leaves with the process
	Ok(())
}
