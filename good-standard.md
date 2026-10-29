# The good standard

good.ftorrent.com, the desktop client's Connection tab, moon, and the public services beneath them are one product with one aim. This note states the aim, so every milestone, and every fork in a design, can be held up against it: whether we're building the right thing with the right focus, which rabbit holes to dig all the way out, and which to walk past.

## The aim

good is the best detector of a consumer internet connection's peer-to-peer connectivity that can be built today. Not a good one, and not one as good as the others: no better connection checker for peer-to-peer should be possible. It measures what a connection actually does, tells apart the kinds of connection that behave differently, and says what each means for the person whose connection it is. The page and the panel each go as far as their platform allows, and moon is the vantage that makes the measurements possible.

## The limits we accept

The limits that bound good are the platforms' and the protocols'. They're what macOS, Windows, iOS, and Android let a program do, what a browser's sandbox and its WebRTC let a page do, and what STUN, ICE, and the translators and firewalls along the path permit. Inside those, every limit is ours to remove. A measurement good could make and doesn't is a gap in the product, unless it serves only a connection rare enough to leave out.

## Real connections, and the corners we leave

good is for the connections real people have today, around the world: a home router on broadband, a phone on a carrier's network, a carrier's translator above a home router, IPv6 where it's deployed and its absence where it isn't, a VPN, an office or a campus. These are the cases to detect, to tell apart, and to explain completely, and the wrinkles they bring are the work itself, never a distraction from it.

A case rare enough that few people will meet it, and that wouldn't change what anyone is told or can do, stays out on purpose. Leaving it out is what makes room to dig the common cases out completely. That's the test for any rabbit hole: dig all the way when the case is common, or when its answer changes what a person is told or can do, and walk past it otherwise.

## Measuring over guarding

When a better measurement needs moon or a peer to send to more places, good takes the better measurement and watches it, rather than giving the measurement up to guard against misuse whose harm is mild. A STUN server can be used as a reflector, and a peer running ICE sends its checks wherever a browser's candidates say. The harm in either is a little reflected traffic, which is unlikely to begin with, which the counters on moon would show, and which the guard that follows them can bound. Where the harm would be real, as with a TCP connection opened to an address someone merely names, good sends only to an address the request itself proves, and still finds a way to take the measurement.

## Using this note

Read it at the start of each milestone, and whenever a design forks. The question it asks is always the same: does this make good a better detector of the connections people actually have?

Good8 met that question once already. A phone whose carrier sent its HTTPS and its UDP out from different addresses couldn't punch at all, under a rule that held moon's peer to the address the HTTPS came from. The rule changed: the phone is a connection the page most needs to read, and a browser's ICE checks carry their own consent, since only that browser can answer them.
