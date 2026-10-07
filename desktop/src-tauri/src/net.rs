use std::fs::{self, File};
use std::io::{Read, Write};
use std::time::Duration;
use sha2::{Digest, Sha256};
use tauri::command;
use crate::run_blocking;

/*
Requests to the web for the page: net_get fetches one https address, and either answers its body as text or, given a path, saves the body to that file and answers its SHA-256. What to fetch, where to save it, and what the answer means, is the page's.

ureq makes the request, a blocking call on run_blocking's pool like the disk commands, with its default TLS: rustls, checking certificates against Mozilla's roots compiled into the program. The answer is treated as coming from anywhere: https only, no redirect followed, the whole request bounded by the page's seconds, and the body by its limit, counted as it arrives and again after gzip is undone. A redirect comes back as its own body, which the page's check turns away; a 4xx or 5xx is an error.

A saved body streams to disk as it arrives, never held whole in memory or carried across to the page, and is hashed on the way, so the page can compare the file with what it expected without reading it back. It's written beside the path under a .part name and renamed over the path only once the last byte is in, so the path never holds a partial download; a failure partway removes the .part.
*/

/// Fetch this https address, giving up after seconds and refusing a body longer than limit bytes; with no save path, answer the body as text, and with one, write the body to that file and answer its SHA-256 in lowercase hex
#[command]
pub async fn net_get(url: String, limit: u64, seconds: u64, save: Option<String>) -> Result<String, String> {
	run_blocking(move || {
		let agent: ureq::Agent = ureq::Agent::config_builder()
			.https_only(true)//never plain http, even if a redirect or a caller asked for it
			.max_redirects(0)//answer a redirect as it is, rather than following it somewhere else
			.timeout_global(Some(Duration::from_secs(seconds)))//the whole request, connecting through the last byte
			.build()
			.into();
		let mut response = agent.get(&url).call().map_err(|e| e.to_string())?;
		let mut body = response.body_mut().with_config().limit(limit + 1).reader()//ureq's limit counts the bytes as they arrive, before gzip is undone; one past ours, since its reader fails any read once the limit is used up, even the last one that would only find the end
			.take(limit + 1);//and take counts them after, one past limit so the checks below can tell a body exactly limit long from a longer one
		if let Some(path) = save { return save_body(&mut body, &path, limit) }
		let mut text = String::new();
		body.read_to_string(&mut text).map_err(|e| e.to_string())?;
		if text.len() as u64 > limit { return Err(format!("the answer ran past {limit} bytes")) }//an error rather than a cut-off body
		Ok(text)
	}).await
}

/// Stream body into path through a .part file beside it, hashing as it goes, and answer the SHA-256 once the file is whole and in place
fn save_body(body: &mut impl Read, path: &str, limit: u64) -> Result<String, String> {
	let part = format!("{path}.part");
	let written = (|| -> Result<String, String> {
		let mut file = File::create(&part).map_err(|e| e.to_string())?;
		let mut hasher = Sha256::new();
		let mut buffer = vec![0u8; 64 * 1024];
		let mut total: u64 = 0;
		loop {
			let count = body.read(&mut buffer).map_err(|e| e.to_string())?;
			if count == 0 { break }
			total += count as u64;
			if total > limit { return Err(format!("the answer ran past {limit} bytes")) }//take let one byte past limit through so this can tell
			hasher.update(&buffer[..count]);
			file.write_all(&buffer[..count]).map_err(|e| e.to_string())?;
		}
		file.sync_all().map_err(|e| e.to_string())?;//on the disk before the rename makes it the real file
		Ok(hasher.finalize().iter().map(|byte| format!("{byte:02x}")).collect())
	})();
	match written {
		Ok(hash) => fs::rename(&part, path).map(|_| hash).map_err(|e| { let _ = fs::remove_file(&part); e.to_string() }),
		Err(e) => { let _ = fs::remove_file(&part); Err(e) }//a timeout, a refusal past limit, or a full disk takes the partial file back
	}
}
