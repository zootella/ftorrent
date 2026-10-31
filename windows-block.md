# Windows Defender Firewall has blocked some features of this app

The first time ftorrent opens a socket that could take connections from outside, Windows blocks what would arrive at it and asks the person at the keyboard what to do. In the desktop client that first happens on the Connection tab's Check: `net_stun` binds a UDP socket to a fixed port on every address, which to the firewall looks like a program getting ready to hear from anyone. The engine is a separate program, so it raises a dialog of its own the first time it listens, and one more the next time, for a reason below.

The dialog, on a computer whose current network is Public:

> **Windows Defender Firewall has blocked some features of this app**
>
> Windows Defender Firewall has blocked some features of ftorrent on all public and private networks.
>
> Name: ftorrent  
> Publisher: ftorrent  
> Path: the installed ftorrent.exe, under `%LOCALAPPDATA%\ftorrent`
>
> Allow ftorrent to communicate on these networks:
>
> ☐ Private networks, such as my home or work network  
> ☑ Public networks, such as those in airports and coffee shops (not recommended because these networks often have little or no security)
>
> [Allow access] [Cancel]

Allow access carries the administrator shield. Public comes pre-checked because the network the program was on when it tried to listen is categorized Public; on a Private network, Private comes pre-checked instead.

## A mini-rant

The dialog asks a person to make a security decision with two words, and both words lead the reader somewhere else. People use "private" and "public" about their things: a private diary, a public post. The dialog uses them about places, the kind of network the computer is connected to, and asks in which places a program may be reached. A reader who skims sees a question about exposure and answers that question instead. The descriptions under each word carry the real meaning, and the descriptions are the part a skimmer skips.

The descriptions also argue with Windows' own defaults. Windows 11 makes every new network Public and labels that choice recommended in Settings, and then this dialog calls allowing a program on Public "not recommended." The person who reads every word is told their home network is Public and that allowing anything on Public is a bad idea, which leaves them no right answer for their own home.

No box means what most people would want to say, either: the internet, or only my own devices. Every rule the dialog writes admits any address. The categories exist for Windows' own sharing, files, printers, media streaming, and network discovery, which serve the neighbors and are right to switch off at the airport. For that job the design is sound, and it came from a hard lesson: Windows XP Service Pack 2 turned the firewall on by default in 2004, after the Blaster and Sasser worms spread from machine to machine through Windows' own listening services, often carried by laptops from one network into the next, and Windows Vista added the network categories so a laptop's exposure could change as it moved from home to the airport. Stretched over every program, though, the same two boxes ask a question the person usually can't answer and the program usually doesn't care about.

So the person who reads carefully does worst. At home, they check Private, uncheck Public, and stay blocked, because Windows calls their home network Public. The person who skims and leaves the pre-checked box alone ends up allowed. That's the strongest argument for ftorrent writing its own rule for both kinds of network, so that how a person reads two words in a dialog stops mattering.

## The correct reading

The boxes name kinds of network the computer might be connected to, not kinds of traffic and not kinds of data. Windows sorts every network it joins into one of three categories, Domain, Private, or Public, which Settings calls the network profile, remembers the answer for each network, and tags every firewall rule with the categories it applies in.

- **Private** is a network you trust, where the other devices are yours: a home or an office.
- **Public** is a network shared with strangers, where any device on the same Wi-Fi can reach yours directly, with no router translating between you: an airport, a café, a hotel.
- **Domain** appears only on a computer joined to a company's domain, which is why most dialogs show two boxes.

Checking a box means: when this computer is on that kind of network, let anyone, near or far, start a connection to this program. In Microsoft's model, Private is the safer box to check and Public the riskier one.

## The first misreading: how exposed my stuff is

The reader takes the words as the sensitivity of their data, as if the dialog asked whether anyone should see their things. Public is fine, since it's public already; private, no thank you; and Cancel, a plain no, feels safest of all. That inverts Microsoft's model, allowing the riskier box and refusing the safer one. The Public line's "not recommended" warns against it, for a reader who gets that far.

## The second misreading: the internet and my home network

The reader takes Public as the internet and Private as the devices at home. ftorrent talks to the internet, so Public yes; it doesn't need anything at home, so Private no. It's an easy reading even for someone who knows networks well, and it's wrong because no box means local. A peer on the internet reaching a computer at home arrives on the home network's adapter, through a port forward on the router over IPv4 or directly over IPv6, so it's governed by the home network's category like everything else that arrives there.

Both misreadings land on the same clicks, Public checked and Private unchecked, which is also what Windows pre-checks on a Public network.

## What the dialog actually does

**The block comes first.** While the dialog is still on the screen, Windows has already written two inbound Block rules for the program, one for TCP and one for UDP, in the current network's category only. "Has blocked some features of ftorrent on all public and private networks" describes that moment: with no rule naming the program, the firewall's default turns away unrequested inbound traffic in every category. The rules' names aren't reliable: the installed copy's are named `ftorrent.exe`, after the file, and a development build's `ftorrent`, after the program's description, so anything looking for them should find them by program path.

**Each category ends in one of three states.**

- An **allow rule**: inbound connections reach the program.
- A **block rule**: inbound is turned away, the dialog doesn't ask again on that kind of network, and the block overrides any allow rule added later, since in Windows Firewall a block rule wins over an allow rule.
- **No rule**: inbound is still turned away by the default, but the dialog asks again the next time, and an allow rule added later works.

**The answers, on a computer whose current network is Public.** A checked box turns the waiting Block rules into Allow rules, and an unchecked box for another category writes nothing, leaving that category with no rule, so the dialog asks again there. Cancel isn't quite "no to both": it leaves the Block rules already written and nothing for Private. Allow access finishes writing the rules before the dialog closes, so the program's window gets focus back after the rules have changed, a little under a second later.

| Answer | Public, the current network | Private |
|---|---|---|
| Cancel | block, the rules already there | no rule (expected) |
| Allow access, Public only | allow | no rule |
| Allow access, Private only | block, kept (expected) | allow (expected) |
| Allow access, both | allow (expected) | allow (expected) |

**Allowing takes an administrator, and refusing doesn't.** Allow access carries the shield, which means it needs administrator rights. On an administrator's account at Windows' default UAC setting there's no prompt at all, since the dialog is part of Windows and Windows' own changes elevate silently for an administrator; a standard user is asked for an administrator's credentials. Cancel asks for nothing, so a standard user without those credentials can only leave the Block rules in place, and firewall rules apply to every account on the computer.

**Rules name a path.** A program at another path is another program to the firewall. ftorrent's one-click update keeps the installed path, so a rule survives updates; a development build or a portable copy is asked about on its own. The rules outlive an uninstall, since an uninstaller running without elevation can't remove them, and a reinstall at the same path inherits them with no dialog.

**The engine is asked twice.** Edge traversal is unrequested traffic arriving through Teredo, the tunnel that carries IPv6 inside IPv4. The rules Allow access writes defer it to the user, so Windows asks again when a program asks for it. libtorrent asks on Windows: in `session_impl::setup_listener` it sets an unrestricted protection level on each IPv6 TCP listen socket, under the comment "enable Teredo on windows." So the engine's next Start brings the dialog back, and Allow access this time defers edge traversal to the program, whose socket has already said yes. After that the engine isn't asked again, whether it stops and starts, starts in a new process, or is replaced at the same path by an update. ftorrent's own sockets never ask for Teredo, so ftorrent itself is asked once.

**What it doesn't touch.** Outbound traffic is allowed by default, and a reply comes back in to the socket that sent the request whatever the inbound rules say, so Check's results are the same however the dialog is answered. Without an allow rule, though, Windows should admit a packet only from an address and port the socket has sent to, which is port-restricted filtering, the same behavior the panel will look for in the router; Good7's filtering check is where that gets measured.

## ftorrent's own rule

Write firewall rules…, on the Connection tab of an installed copy, writes the rule this note argues for. One UAC prompt approves a run of netsh that deletes every inbound rule naming ftorrent.exe or the engine, the dialog's Block rules among them, and adds an allow for each over TCP and UDP on every kind of network, with edge traversal deferred to the program, so the engine's second dialog never comes.

The prompt names Windows PowerShell, with Microsoft Windows as its verified publisher, rather than ftorrent, so the Connection tab says what's about to happen before the prompt appears. Its details show the netsh commands as written. The prompt asks a standard user for an administrator's credentials, as Allow access does. So ftorrent's rule takes the same rights the dialog does, and what it buys is every kind of network at once and ftorrent's own words before the prompt.

Firewall settings…, beside it, opens Control Panel's Allow an app through Windows Firewall. That page lists rules by name, the TCP and UDP rules of one name as a single row, with a box for Private and one for Public, so the alert's rules show as ftorrent.exe and ftorrent-engine.exe, and ours as ftorrent and ftorrent engine.

## Why most Windows home networks are probably Public

We found no data: no Microsoft figures, survey, or study of how many home networks are categorized Public. The defaults point one way.

- **Windows 11** makes every new Wi-Fi network Public, and its Settings label the choice "Public network (Recommended)," advising Private only where file or printer sharing is needed.
- **Windows 10** asks once, on the first connection, whether the PC should be discoverable by other devices. Yes makes the network Private; no, or dismissing the question, leaves it Public.
- **Changing it later** takes a trip into Settings, under Network & internet and the network's properties, which people mostly make when a printer or a file share won't work.
- **The Windows 10 machine this was written on** has its Ethernet connection categorized Public.

Some home networks are Private all the same: networks carried over from Windows 7 or 8, whose "Home network" choice became Private and survives upgrades; the networks of people who set up file sharing, a media server, or a network printer; and those of anyone who answered yes when asked. That's a real minority whose size we can't measure. So the working assumption is Public as the common case at home and Private as a sizable, unmeasured minority, which is why ftorrent's own rule should cover both.

Sources:

- [Microsoft Support: Make a Wi-Fi network public or private in Windows](https://support.microsoft.com/en-us/kb/4043043)
- [Windows Central: How to configure network discovery on Windows 10](https://www.windowscentral.com/how-configure-network-discovery-windows-10)
- [Geek Rewind: How to make your network public or private in Windows 11](https://geekrewind.com/how-to-make-your-network-public-or-private-in-windows-11/)
