use std::io::Read;
use std::time::Duration;
use tauri::command;
use crate::run_blocking;

/*
Requests to the web for the page: net_get fetches one https address and answers its body as text. What to fetch, and what the answer means, is the page's.

ureq makes the request, a blocking call on run_blocking's pool like the disk commands, with its default TLS: rustls, checking certificates against Mozilla's roots compiled into the program. The answer is treated as coming from anywhere: https only, no redirect followed, the whole request bounded by the page's seconds, and the body by its limit, counted as it arrives and again after gzip is undone. A redirect comes back as its own body, which the page's check turns away; a 4xx or 5xx is an error.
*/

/// Fetch this https address and answer its body as text, giving up after seconds and refusing a body longer than limit bytes
#[command]
pub async fn net_get(url: String, limit: u64, seconds: u64) -> Result<String, String> {
	run_blocking(move || {
		let agent: ureq::Agent = ureq::Agent::config_builder()
			.https_only(true)//never plain http, even if a redirect or a caller asked for it
			.max_redirects(0)//answer a redirect as it is, rather than following it somewhere else
			.timeout_global(Some(Duration::from_secs(seconds)))//the whole request, connecting through the last byte
			.build()
			.into();
		let mut response = agent.get(&url).call().map_err(|e| e.to_string())?;
		let mut text = String::new();
		response.body_mut().with_config().limit(limit).reader()//limit counts the bytes as they arrive, before gzip is undone
			.take(limit + 1).read_to_string(&mut text).map_err(|e| e.to_string())?;//and take counts them after, one past limit so a body exactly limit long still fits
		if text.len() as u64 > limit { return Err(format!("the answer ran past {limit} bytes")) }//an error rather than a cut-off body
		Ok(text)
	}).await
}
