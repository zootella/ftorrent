# File associations on Windows, measured against uTorrent

A record of what we measured when we ran ftorrent beside an older BitTorrent client on Windows 10 and watched the two contend for the `.torrent` file type and the `magnet:` link scheme. The first half is background: how Windows decides which program opens a file, how that decision has been guarded since Windows 8, and where the guard holds and where it doesn't. The second half is the measurements, each one a thing we observed on a real machine and can state plainly. The design ftorrent follows is in the desktop client's planning document under file and protocol associations; this page is about the ground that design stands on.

## How Windows decides what opens a file

Three layers of the registry answer the question "what opens a `.torrent`?", and the shell reads them in order.

**The fallback.** Under `Software\Classes\.torrent`, the key's own default value names a ProgID, like `uTorrent` or `ftorrent.torrent`, and that ProgID's `shell\open\command` names the program. Any program may write this, per user under `HKEY_CURRENT_USER` or for the whole machine under `HKEY_LOCAL_MACHINE`. This was the entire mechanism from Windows 95 through Windows 7, and the habit it taught installers, write the default at install and check it at every launch, is still in much association code today.

**The offer.** Beside the fallback, `OpenWithProgids` under the same key lists every ProgID that can open the type. It takes nothing from anyone; it is the list Explorer's Open with menu is built from. Windows Vista added a second kind of offer, a `Capabilities` block named in `RegisteredApplications`, which is what puts a program by name into Settings under Default apps.

**The user's choice.** Above both, per user, Windows 8 added `UserChoice`, under Explorer's `FileExts\.torrent` for a file type and under `Shell\Associations\UrlAssociations\magnet` for a link scheme. It holds the ProgID the user chose and a hash. Only Windows' own screens write it: Settings, Default apps, and the "always use this app" box in Explorer. When it exists and its hash checks, it decides, and the fallback is ignored.

## How the user's choice is guarded, and how it isn't

The hash is computed from the extension or scheme, the user's security identifier, the ProgID, the time to the minute, and a fixed secret string, by an algorithm Microsoft has never published. Two things enforce it. A program that writes `UserChoice` with a wrong hash is ignored, and Windows 10 goes further: it notices the mismatch, resets the type, and tells the user "An app default was reset". And the key carries a deny on the user's own account for setting values, so even a program running as the user cannot overwrite the ProgID through ordinary registry calls.

The guard has two gaps, and both are structural rather than bugs.

The first is deletion. The deny covers setting values, not deleting the key, and the user owns the key. A program running as the user can delete `UserChoice` outright, which drops the type back to the fallback, which that program has just written. No toast appears, because there is no bad hash to notice; there is simply no choice any more.

The second is the hash itself. It is a keyed hash whose key, the secret string, has to be known to the code on the machine that computes it, and a secret that code on the machine must know is a secret every program on the machine can learn. The algorithm was reverse-engineered in 2017 and is in a freely available tool, and some installers carry it. This is why the scheme is deterrence rather than security: a real signature would need a key that programs running as the user cannot read, and on a personal computer everything, Settings included, runs as the user. Microsoft's choice was to make seizing a default a deliberate act rather than an accident of an old installer, and to make the deliberate act something a published app can be held to. That is a policy boundary, not a cryptographic one.

Windows 11 added a further layer in 2021 for the browser and web types specifically, and left file types like `.torrent` and schemes like `magnet:` as Windows 10 had them.

## What we measured

The client on the other side was uTorrent 3.5.5 build 46096, from 2021, run with its queued updates removed and its network access blocked by firewall rules, so that nothing it did came from a server. The machine was Windows 10 Pro 22H2. ftorrent was 0.1.0, installed, with its answer set to ask. Everything below was read from the registry and from the shell's own lookup, `AssocQueryString`, between steps.

**Launching uTorrent alone rewrote the shared `magnet` class.** Before its default-program prompt was answered, with the prompt still on screen, `Software\Classes\magnet\shell\open\command` named uTorrent. The `.torrent` fallback and both `UserChoice` keys were untouched at that point.

**Answering yes to its prompt deleted the user's choice.** `FileExts\.torrent\UserChoice`, which had named ftorrent, was gone. So was `Software\Classes\.torrent\OpenWithProgids`, with ftorrent's offer in it, and Explorer's copy of that list. The `.torrent` fallback now named `uTorrent`. The `UserChoice` for `magnet`, which also named ftorrent, was left alone, so Windows went on opening magnet links with ftorrent while uTorrent held the shared class beneath. The associate button in uTorrent's preferences made the same writes again, on demand.

**With the user's choice gone, the shell's answer turned on Explorer's recent-use list.** `FileExts\.torrent\OpenWithList` records the programs a user has picked through Open with, most recent first. We put the registry through six states and asked the shell each time, with no `UserChoice` and the fallback naming uTorrent throughout:

| Open-with list under `Software\Classes\.torrent` | Recent-use list, first entry | Shell answers |
|---|---|---|
| absent | ftorrent | uTorrent |
| `uTorrent` only | ftorrent | uTorrent |
| `ftorrent.torrent` only | ftorrent | ftorrent |
| both | ftorrent | ftorrent |
| both | qBittorrent | uTorrent |
| both, ftorrent dropped from `RegisteredApplications` | ftorrent | ftorrent |

So on Windows 10, where no choice is saved, the shell prefers the most recently used Open-with program when a ProgID in the type's Open-with list runs it, and falls to the fallback otherwise. Being a registered application with a `Capabilities` block made no difference to this. We measured this for one extension on one machine; we have not found it documented.

**ftorrent held `.torrent` without claiming it.** On each return to its window, ftorrent renews its offer, and the first pass after uTorrent's writes put `ftorrent.torrent` back into the Open-with list it had deleted. That alone flipped the shell's answer back to ftorrent, because this user had once opened a `.torrent` with ftorrent through Open with. ftorrent wrote no fallback and deleted nothing. Its bar, which asks only when the shell opens a type with another program, never appeared, because by the time each pass read the shell, the shell answered ftorrent. A double-click on a `.torrent` and a `magnet:` link entered at Run both reached the running ftorrent through its handoff while uTorrent was running beside it.

On a machine whose recent-use list has never seen ftorrent, the same writes by uTorrent would stick until the user chose again, ftorrent's bar would come up, and a yes would send the user to Windows' Default apps, where the choice is saved above the fallback. Against a program that deletes that choice at every launch, no well-behaved program's registration holds, and the honest move is the one ftorrent makes: read what the shell says, and ask again.
