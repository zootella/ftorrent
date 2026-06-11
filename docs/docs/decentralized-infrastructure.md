---
title: Open Infrastructure for the Decentralized Web
description: Open, independent infrastructure for the decentralized web — what it is, why it matters, and what ftorrent runs.
---

# Open Infrastructure for the Decentralized Web

notes

services provided
- udp tracker
- http tracker
- webrtc signaling server
- dht bootstrap node

each of those, talk about what these things are
- how they work
- how bittorrent and webtorrent uses them (in here is how webtorrent uses web sockets, webrtc signalling server, how peers connect, what they can and cannot do from a browser tab's sandbox)
- how your app that has nothing to do with bittorrent could use them

and then argue
- the tendency to reinvent and reimplement, unnecessarily (but also why projects do that, more buzz, less responsibility to get to functional)
- the value of using an existing traveled road (proof it works, scales, many good and proven implementations to choose from)
- bring a good citizen, use, but also run, this infrastructure, avoid the tradegy of the commons
