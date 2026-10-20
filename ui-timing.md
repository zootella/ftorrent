# UI timing

How often the desktop client asks the engine what happened, and how often what's on the screen changes. The Connection tab starts at once a second; this note holds the question of whether that's right for the client as a whole, and what the answer turns on.

## The question

Most BitTorrent clients redraw on a steady beat, about once a second, and a person watching the transfer list feels it as a drumbeat. Is that the right beat for ftorrent, and can it be tuned?

- Twice a second would make ftorrent look twice as quick as the clients beside it.
- If a faster beat costs nothing, ten times a second would show news about as fast as it arrives on the wire.
- Every frame is more than anything here needs.
- A fast beat that varies, rather than a steady one, wouldn't settle into a rhythm that draws the eye.

## How news reaches the screen

- libtorrent does its work on its own threads and reports what happens as alerts, which wait in a queue inside the session until something asks for them. The queue holds 2,000 alerts by default and drops the rest, posting an `alerts_dropped_alert` to say so.
- libtorrent keeps its own clock too: an internal tick every 500 milliseconds by default, its `tick_interval`, on which it hands out bandwidth and updates its rates and counters.
- The engine is one loop with no threads. It does work only when a line arrives from the page, so the page sets the pace: it sends a poll on an interval, and the engine answers with every alert waiting.
- The engine's lines wait in a queue in Rust until the page takes them, every 250 milliseconds, in `desktop/src/stores/incoming.js`.
- qBittorrent refreshes every 1,500 milliseconds by default, a number a person can change in its advanced settings.

So news waits twice on its way up: up to one poll interval in libtorrent's queue, and up to 250 milliseconds in Rust's. At a one-second poll, an event reaches the page somewhere between a moment and a second and a quarter after it happened.

## What a faster poll costs

Very little in the road itself, though we haven't measured it yet. A poll is one short line down and a line up for each alert, and popping libtorrent's queue is quick, so we expect ten polls a second to cost a small fraction of a core. The cost lives at either end. On the page, every change re-renders what shows it, and a list of a few hundred torrents redrawn ten times a second would likely cost more than the pipe ever will. In the engine, a stats request walks every counter libtorrent keeps, three hundred of them, which is cheap once a second and wasteful ten times a second. And a faster poll can't make a rate fresher than the tick it was computed on.

A hidden window needs news less often, but it can't stop asking entirely: libtorrent keeps working whether or not anyone is looking, and a queue nobody pops fills and starts dropping alerts. So a hidden window slows down rather than stopping.

## Two clocks, not one

The beat a person feels comes from tying two things together that don't have to be tied: how fresh the page's news is, and how often the screen moves. Pulled apart, each can be set by what it's for.

- **Ask fast.** Polling every 100 to 250 milliseconds costs little and gets events to the page soon after libtorrent posts them.
- **Show events the moment they arrive.** A peer connecting, a torrent finishing, a file added: these are things that happened, and showing them at once is what makes a client feel quick.
- **Change a number only when it changes.** A count of peers moves when a peer comes or goes, not on a schedule. A rate changes all the time and jumps around when measured over a short window, so it's smoothed before it's shown and redrawn no faster than libtorrent's tick.

With the screen following events instead of a timer, there's no beat to hide, which answers the variable-interval idea a different way: varying the interval hides a rhythm, but a number that changes at random moments reads as unsteady, and a screen that changes only when something happened has no rhythm in the first place.

The speed a person perceives is real but narrow. A faster page shows the same transfer sooner; it doesn't make the transfer faster. That's still worth having, since being told promptly is part of feeling in control, and it's worth describing honestly wherever ftorrent talks about it.

## For now

We start the Connection tab's session poll at one second, the beat people already know from other clients, and keep the interval in one constant so changing it is one edit. The real decision comes with the transfer list, which is where the beat is felt, and where showing events at once and smoothing rates can be tried against a real swarm.
