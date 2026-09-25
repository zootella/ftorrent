//./src-tauri/src/lifecycle.rs

use tauri::{AppHandle, Manager, Window, WindowEvent};

/*
ftorrent keeps running with its window closed, the way a file transfer application does: closing the window is putting it away, and quitting is a separate, explicit act. There is no setting for this. One process, one window, on every platform: a second launch never opens another window, it hands over to this one (instance.rs), and this module is how that one window goes away and comes back.

Closing hides. The close button on Windows and the red button on macOS hide the window instead of closing it, so the page and its webview stay alive, keep their state, and come back at once; the webview's footprint is small next to the engine's. The window is created once when ftorrent starts and destroyed once when it quits.

Quitting is on each platform's own terms. On macOS it's Quit in the app menu, ⌘Q, or Quit in the Dock icon's menu, all of which macOS provides; and clicking the Dock icon while the window is hidden sends Reopen, which brings it back. On Windows there's no Dock, so a tray icon stands in: clicking it brings the window back, and its menu has Show and Exit. Every way of quitting reaches the Exit run event in lib.rs, which stops the engine.

Linux keeps closing as quitting for now. It has no tray here and no handoff yet, so a hidden window would be one nobody could get back.
*/

/// Show the window, restore it if it's minimized, and give it focus; the tray, the Dock, and a second launch all bring ftorrent back this way
pub fn bring_forward(app: &AppHandle) {
	if let Some(window) = app.get_webview_window("main") {
		let _ = window.show();
		let _ = window.unminimize();
		let _ = window.set_focus();
	}
}

/// The close button hides the window rather than closing it, on macOS and Windows
pub fn window_event(window: &Window, event: &WindowEvent) {
	if cfg!(any(target_os = "macos", target_os = "windows")) {
		if let WindowEvent::CloseRequested { api, .. } = event {
			api.prevent_close();//the window stays, and so does everything behind it
			let _ = window.hide();
		}
	}
}

/// On Windows, the tray icon that brings the window back and quits; called once from setup
#[cfg(target_os = "windows")]
pub fn tray_install(app: &AppHandle) -> tauri::Result<()> {
	use tauri::menu::{Menu, MenuItem};
	use tauri::tray::{MouseButton, MouseButtonState, TrayIconBuilder, TrayIconEvent};
	let show = MenuItem::with_id(app, "show", "Show ftorrent", true, None::<&str>)?;
	let exit = MenuItem::with_id(app, "exit", "Exit", true, None::<&str>)?;
	let menu = Menu::with_items(app, &[&show, &exit])?;
	let mut tray = TrayIconBuilder::with_id("main")
		.tooltip("ftorrent")
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
