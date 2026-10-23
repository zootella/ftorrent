# Caffeinate

A record of the last thirty days, a minute at a time, of when the computer was awake, when it reached the internet, and when ftorrent was on the network, drawn so a person can read a month at a glance. It comes before any setting that holds a computer awake or online: first the person sees what their computer actually does, and then ftorrent offers to change it, with the record there to show whether the change held. The name is from `caffeinate`, the command macOS ships for holding a Mac awake. Started 2026-10-23, during the good sprint, as notes ahead of any plan.

## The story

An app offers to keep the computer from falling asleep. The person turns it on, walks away, and comes back hours later to a computer that slept anyway, with nothing to say when it went down, or why, or whether the app ever held it up. When that happens to ftorrent, the person has a durable record of exactly when each part was up, minute by minute, for the last thirty days. So the diagnostic comes first, and settings like keeping the computer online come after, on top of a record that shows whether they work.

## Three lines

- **The computer.** Awake: the processor and memory running and the network card powered, so a packet can arrive. Not asleep, not hibernating, not off.
- **The internet connection.** Evidence the computer reached the internet that minute: something that came back from past the router, like a STUN answer, a DHT reply, or a tracker's response.
- **ftorrent.** On the network: running, in the DHT, and seeding.

Each line can drop while the one above it holds. A computer can be awake on a connection that's down, and a connection can be up while ftorrent isn't running, so the record keeps the three apart and the picture draws them as three rows.

## Notes

- Time since boot is the first number toward the computer line, and the Connection tab can read it: `kern.boottime` from `sysctl` on a Mac, `/proc/uptime` on Linux, and GetTickCount64 on Windows. A Mac also records its last wake, `kern.waketime`.
- Windows' Fast Startup, on by default, makes Shut down hibernate the kernel rather than end it, so the next start resumes it. Time since boot resets only on Restart, and a computer shut down every night can show weeks of uptime in Task Manager.
- The network rejoins at every wake. Wi-Fi reassociates, the DHCP lease is renewed, IPv6 may make new temporary addresses, and the mappings the router held for the computer have likely timed out while it slept. The engine's sockets survive a sleep; the router's state about them may not.
- Each system has a way for a program to hold it awake, and each stops short of what a person might expect.
	- macOS has power assertions, which `caffeinate` makes. Preventing idle sleep doesn't stop a laptop sleeping when its lid closes, and preventing system sleep, `caffeinate -s`, holds only on AC power. `pmset -g assertions` lists every assertion held and which program holds it.
	- Windows has SetThreadExecutionState and power requests, PowerCreateRequest and PowerSetRequest. Neither stops a sleep the person asks for or a lid closing. `powercfg /requests` lists what's holding the computer awake.
	- Linux has systemd-logind's inhibitor locks, which `systemd-inhibit` takes from the command line.
- Other things put a computer down or wake it: Windows Update restarting outside the active hours; Modern Standby on newer Windows laptops, where sleep is a low-power state of the running system, with the network kept connected or not by a setting; and on a Mac, Power Nap's short wakes for maintenance, with the network up.
- A Mac slows the timers of an app nobody is looking at, App Nap, and a web view can slow the timers of a page that's hidden, so whether ftorrent's once-a-second poll keeps its beat with the window hidden is a question to measure.
- ftorrent can write its own line for each minute it runs, from what it already knows: whether the engine's session is up, the DHT's node count, the torrents seeding, and the counters that show packets arriving. That covers the internet and ftorrent lines for the minutes ftorrent ran, and nothing for the minutes it didn't.
- The systems keep records that fill in the rest.
	- macOS's power log, `pmset -g log`, lists every sleep and wake with its reason, days back.
	- Windows' System event log holds Kernel-Power events for sleep and resume, and Power-Troubleshooter events naming what woke the computer; `powercfg /lastwake` reads the last one, and on a Modern Standby machine `powercfg /sleepstudy` reports the days before.
	- Linux's journal lists each boot, `journalctl --list-boots`, and systemd's lines for each sleep and resume. Whether the Flatpak can read it is a question.
- The router keeps its own clocks. NAT-PMP's epoch counts the seconds since the router's mapping table started, so it resets when the router restarts, and on a router that speaks UPnP, GetStatusInfo reports how long its internet connection has been up. Good5 reads both.
- Thirty days is 43,200 minutes. A few bits a minute for each line is a small file of its own in the data folder.
