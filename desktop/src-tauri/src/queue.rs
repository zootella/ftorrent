use std::collections::VecDeque;
use serde::Serialize;

/*
A drained queue: things land here on a thread of their own, and the page takes them all at once. The engine's lines and the requests that reach this copy both arrive this way, before the page is up or between its looks, and the page needs every one of them, in order.

Push appends. Take hands over everything waiting and leaves the queue empty, so the page reads each item exactly once and keeps its own history of what it cares about. The queue has a cap, CAP, so a page that stops taking, say a hidden web view whose timers the system has slowed, can't make memory grow without end: a push that would pass the cap drops the oldest item and counts it, and the next take reports the count, so the page can say something was lost rather than lose it silently.

It knows nothing about what it holds.
*/

/// What a take hands the page: everything waiting, oldest first, and how many were dropped since the last take because the queue was full
#[derive(Serialize)]
pub struct Drained<T> {
	pub items: Vec<T>,
	pub dropped: u64,
}

/// A queue that holds at most CAP items, dropping and counting the oldest to make room
pub struct Queue<T, const CAP: usize> {
	items: VecDeque<T>,
	dropped: u64,
}

impl<T, const CAP: usize> Default for Queue<T, CAP> {
	fn default() -> Self { Queue { items: VecDeque::new(), dropped: 0 } }
}

impl<T, const CAP: usize> Queue<T, CAP> {
	/// Add an item at the end, making room by dropping the oldest if the queue is full
	pub fn push(&mut self, item: T) {
		if self.items.len() >= CAP { self.items.pop_front(); self.dropped += 1; }
		self.items.push_back(item);
	}

	/// Everything waiting and the dropped count, leaving the queue empty and the count at zero
	pub fn take(&mut self) -> Drained<T> {
		Drained { items: std::mem::take(&mut self.items).into(), dropped: std::mem::take(&mut self.dropped) }
	}
}
