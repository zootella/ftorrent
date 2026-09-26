use tauri::{AppHandle, Manager, Window, WindowEvent};

/*
ftorrent keeps running with its window closed, the way a file transfer application does: closing the window is putting it away, and quitting is a separate, explicit act. There is no setting for this. One process, one window, on every platform: a second launch never opens another window, it hands over to this one (instance.rs), and this module is how that one window goes away and comes back.

Closing hides. The close button on Windows and the red button on macOS hide the window instead of closing it, so the page and its webview stay alive, keep their state, and come back at once; the webview's footprint is small next to the engine's. The window is created once when ftorrent starts and destroyed once when it quits.

Quitting is on each platform's own terms. On macOS it's Quit in the app menu, ⌘Q, or Quit in the Dock icon's menu, all of which macOS provides; and clicking the Dock icon while the window is hidden sends Reopen, which brings it back. On Windows there's no Dock, so a tray icon stands in: clicking it brings the window back, and its menu has Show and Exit; and the window has a File menu with Exit, the way a Windows client's File menu ends, for whoever never looks at the tray. Every way of quitting reaches the Exit run event in lib.rs, which stops the engine and, on Windows, takes the tray icon down before the process goes.

Focus is taken only when the user asks for the window. bring_forward shows the window, restores it if it's minimized, and gives it focus, and it runs only for a second launch's handoff, the tray, and the Dock, each of which begins with the user reaching for ftorrent; without the focus, a window brought back by a handoff on Windows can land behind whatever the user was just in. The app never takes focus on its own: at startup the page shows the window and leaves it to the operating system whether a newly launched app comes to the front, which it gets right for the Dock, Finder, Spotlight, and the Start menu, and holds back, on purpose, for a launch it didn't see the user make.

Linux keeps closing as quitting for now. It has no tray here and no handoff yet, so a hidden window would be one nobody could get back. The page listens for the close too, to save where the window was, and in Tauri a page that listens decides whether the close goes ahead. It always declines, so that on macOS and Windows the hide is the only thing a close does. So on Linux, Rust turns the close into a quit, which reaches the same Exit run event, where the text the page handed down for that moment is written.
*/

/// Show the window, restore it if it's minimized, and give it focus; the tray, the Dock, and a second launch all bring ftorrent back this way
pub fn bring_forward(app: &AppHandle) {
	if !app.state::<crate::window::Revealed>().0.load(std::sync::atomic::Ordering::SeqCst) { return }//the page hasn't placed the window yet, and shows it itself in a moment; window.rs says why this waits
	if let Some(window) = app.get_webview_window("main") {
		let _ = window.show();
		let _ = window.unminimize();
		let _ = window.set_focus();
	}
}

/// The close button hides the window rather than closing it, on macOS and Windows, and quits on Linux
pub fn window_event(window: &Window, event: &WindowEvent) {
	if let WindowEvent::CloseRequested { api, .. } = event {
		if cfg!(any(target_os = "macos", target_os = "windows")) {
			api.prevent_close();//the window stays, and so does everything behind it
			let _ = window.hide();
		} else {
			window.app_handle().exit(0);//linux: the page declined the close, so quitting is said outright; Exit writes the settings and stops the engine
		}
	}
}

/// On Windows, the menu bar: File, and Exit under it. The close button hides, so this is the in-window way to quit, the one that needs no tray icon and that keyboard users reach; µTorrent, qBittorrent, and Deluge all have it. Called once from setup
#[cfg(target_os = "windows")]
pub fn menu_install(app: &AppHandle) -> tauri::Result<()> {
	use tauri::menu::{Menu, MenuItem, Submenu};
	let exit = MenuItem::with_id(app, "exit", "Exit", true, None::<&str>)?;//the same id as the tray's Exit, so both reach one handler; no shortcut, the way Exit reads in a Windows File menu, since the system's own Alt+F4 is a close, which now hides
	let file = Submenu::with_items(app, "File", true, &[&exit])?;
	app.set_menu(Menu::with_items(app, &[&file])?)?;//on windows, a menu set on the app is the menu bar of its window
	app.on_menu_event(|app, event| if event.id().as_ref() == "exit" { app.exit(0) });//reaches the Exit run event, which stops the engine; the tray's menu has its own handler, and its Show never comes here
	Ok(())
}

/// On Windows, take the tray icon down before the process ends; called from the Exit run event, which every way of quitting reaches. Dropping the icon is what sends the shell its remove message; a process that just exits leaves a ghost icon in the notification area until the mouse touches it
#[cfg(target_os = "windows")]
pub fn tray_remove(app: &AppHandle) {
	let _ = app.remove_tray_by_id("main");//none means it was never built, or is already gone
}

/// On Windows, the tray icon that brings the window back and quits; called once from setup
#[cfg(target_os = "windows")]
pub fn tray_install(app: &AppHandle) -> tauri::Result<()> {
	use tauri::menu::{Menu, MenuItem};
	use tauri::tray::{MouseButton, MouseButtonState, TrayIconBuilder, TrayIconEvent};
	let brand = &app.package_info().name;//the product name from tauri.conf.json
	let show = MenuItem::with_id(app, "show", format!("Show {brand}"), true, None::<&str>)?;
	let exit = MenuItem::with_id(app, "exit", "Exit", true, None::<&str>)?;
	let menu = Menu::with_items(app, &[&show, &exit])?;
	let mut tray = TrayIconBuilder::with_id("main")
		.tooltip(brand)
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
	if let Some(icon) = app.default_window_icon() { tray = tray.icon(icon.clone()) }//the app icon until the icons story draws a small one for the tray
	tray.build(app)?;//the app keeps it, by its id, for as long as ftorrent runs
	Ok(())
}
