use tauri::command;
use crate::run_blocking;

/*
Launch Services, the part of macOS that decides which app opens a file or a link, offered to the page the way registry.rs offers the Windows registry: general commands that any Mac application could use as they are, knowing nothing about what's asked or why. Which types, which app, and when is the page's; associate.js is the one that uses these today.

A type is named the way the page names it on every platform: a file extension with its dot, like .torrent, or a link scheme without its colon, like magnet. On the Mac a file type is really a type identifier, and an extension names one, so launch_opens and launch_claim look up the identifier the system knows for that extension and use it; a link is just its scheme.

launch_opens asks which app the system would open that type with right now, and answers the app's path, or nothing when no app would. It's the same lookup Finder makes, through NSWorkspace, so it reflects the user's own choice wherever they made it: Finder's Get Info, another app's button, or this one.

launch_claim makes an app the default for that type. It takes the app by its path, but what macOS records is the app's bundle identifier, so where several copies share one, as an installed copy, a build in a repository, and a mounted disk image do, macOS chooses which of them opens, and launch_opens answers with the one it would choose. macOS asks the user nothing for a file type or for a link scheme other than the web's, and answers once the change is made, which is when this answers too, or after CLAIM_WAIT if the answer never comes, so the page's promise always settles. There's no command to give a type back: a default on the Mac always names some app, and macOS has no way to unset one. On Windows and Linux each command answers that there's no Launch Services.

Both commands wait on another program, the Launch Services daemon, and neither waits on the thread that runs the window. launch_opens runs its body through run_blocking in lib.rs, since the lookup blocks until the daemon answers. launch_claim is async in its own right: its body asks and returns at once, and macOS's answer arrives on a channel, so the wait holds no thread at all.
*/

/// Which app the system would open a file type like .torrent or a link scheme like magnet with right now, by its path; nothing when no app would
#[command]
pub async fn launch_opens(name: String) -> Result<Option<String>, String> {
	run_blocking(move || platform::opens(&name)).await
}

/// Make the app at this path the default for a file type like .torrent or a link scheme like magnet
#[command]
pub async fn launch_claim(name: String, app: String) -> Result<(), String> {
	platform::claim(&name, &app).await
}

#[cfg(target_os = "macos")]
mod platform {
	use std::time::Duration;
	use block2::RcBlock;
	use objc2::rc::Retained;
	use objc2_app_kit::NSWorkspace;
	use objc2_foundation::{NSError, NSString, NSURL};
	use objc2_uniform_type_identifiers::UTType;

	const CLAIM_WAIT: Duration = Duration::from_secs(10);//how long launch_claim waits for macOS's answer; it comes at once in practice, so this only keeps a lost answer from leaving the page's promise unsettled for good

	enum Kind {
		File(Retained<UTType>),//a file type, by the identifier its extension names
		Link(Retained<NSString>),//a link scheme
	}

	fn kind(name: &str) -> Result<Kind, String> {//a leading dot means a file extension, and anything else a scheme
		match name.strip_prefix('.') {
			Some(extension) => UTType::typeWithFilenameExtension(&NSString::from_str(extension)).map(Kind::File).ok_or_else(|| format!("launch: no type for {name}")),
			None => Ok(Kind::Link(NSString::from_str(name))),
		}
	}

	pub fn opens(name: &str) -> Result<Option<String>, String> {
		objc2::rc::autoreleasepool(|_| {//a pool for this thread, which tokio's threads don't have and every thread that calls into Cocoa needs: whatever macOS autoreleases while answering is let go of here, at the end of the call, rather than piling up on the thread
			let workspace = NSWorkspace::sharedWorkspace();
			let app = match kind(name)? {
				Kind::File(file) => workspace.URLForApplicationToOpenContentType(&file),
				Kind::Link(_) => {
					let link = NSURL::URLWithString(&NSString::from_str(&format!("{name}:"))).ok_or_else(|| format!("launch: {name} isn't a scheme"))?;//a bare link in the scheme is enough to ask about
					workspace.URLForApplicationToOpenURL(&link)
				}
			};
			Ok(app.and_then(|url| url.path()).map(|path| path.to_string()))
		})
	}

	pub async fn claim(name: &str, app: &str) -> Result<(), String> {
		let mut receiver = ask(name, app)?;//every object of macOS's is made and let go of in there, since none of them may be held across the wait below
		match tokio::time::timeout(CLAIM_WAIT, receiver.recv()).await {
			Ok(answer) => answer.unwrap_or_else(|| Err("launch: macOS let go of the answer without giving one".to_string())),//the channel closed, which means the block was released uncalled
			Err(_) => Err(format!("launch: no answer from macOS in {} seconds", CLAIM_WAIT.as_secs())),//a late answer finds nobody listening and is dropped by the block
		}
	}

	fn ask(name: &str, app: &str) -> Result<tauri::async_runtime::Receiver<Result<(), String>>, String> {//ask macOS to make the change, and hand back where its answer will arrive
		objc2::rc::autoreleasepool(|_| {//the same pool as in opens, since this runs on a tokio worker, which has none either
			let workspace = NSWorkspace::sharedWorkspace();
			let application = NSURL::fileURLWithPath(&NSString::from_str(app));
			let (sender, receiver) = tauri::async_runtime::channel::<Result<(), String>>(1);//macOS answers on a queue of its own, and this carries the answer back
			let done = RcBlock::new(move |error: *mut NSError| {
				let answer = match unsafe { error.as_ref() } {//null when the change was made
					None => Ok(()),
					Some(error) => Err(format!("launch: {}", error.localizedDescription())),
				};
				let _ = sender.try_send(answer);//called once, so there's always room
			});
			match kind(name)? {
				Kind::File(file) => workspace.setDefaultApplicationAtURL_toOpenContentType_completionHandler(&application, &file, Some(&done)),
				Kind::Link(scheme) => workspace.setDefaultApplicationAtURL_toOpenURLsWithScheme_completionHandler(&application, &scheme, Some(&done)),
			}
			Ok(receiver)
		})
	}
}

#[cfg(not(target_os = "macos"))]
mod platform {
	const NONE: &str = "launch: there's no Launch Services on this platform";
	pub fn opens(_name: &str) -> Result<Option<String>, String> { Err(NONE.to_string()) }
	pub async fn claim(_name: &str, _app: &str) -> Result<(), String> { Err(NONE.to_string()) }
}
