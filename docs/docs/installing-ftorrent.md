---
title: Installing ftorrent
description: How to download, install, and open ftorrent on Windows and macOS, how to let every program from the web run, how to get it with one hash-checked command, why ftorrent is not signed through Microsoft's or Apple's programs, and how to check a download against its published hash.
---

# Installing ftorrent

ftorrent for Windows and macOS comes directly from ftorrent.com as two files: `ftorrent.exe`, an installer for 64-bit Intel and AMD PCs, and `ftorrent.dmg`, a disk image for Macs with Apple silicon. Neither is in an app store, and neither is signed with a certificate from Microsoft or Apple. Both systems notice, and the first time you run ftorrent, each one stops and asks you to confirm. This page walks through that confirmation on each system, shows the settings that let every program from the web run without it, gives a command that gets ftorrent with no first-run prompt, explains why ftorrent ships this way, and ends with how to check that the file you downloaded is exactly the one we published.

<DownloadLink file="ftorrent.exe"            platform="Windows" system="64-bit Intel and AMD" />
<DownloadLink file="ftorrent.dmg"            platform="macOS"   system="Apple silicon" />
<DownloadLink file="ftorrent.x86_64.flatpak" platform="Linux"   system="any distribution, sandboxed, 64-bit Intel and AMD" />
<DownloadLink file="ftorrent.amd64.deb"      platform="Linux"   system="Debian and Ubuntu on 64-bit Intel and AMD" />
<DownloadLink file="ftorrent.arm64.deb"      platform="Linux"   system="Raspberry Pi, and other ARM machines running Debian or Ubuntu" />
<DownloadLink file="ftorrent.x86_64.rpm"     platform="Linux"   system="Fedora and RHEL on 64-bit Intel and AMD" />

## What the check looks at

- A browser marks every file it downloads. Windows calls the mark the Mark of the Web, and macOS calls it quarantine. It is an attribute stored with the file, recording that the file came from the internet.
- The first time a marked program runs, the system checks it: SmartScreen on Windows, Gatekeeper on macOS.
- The check asks who published the program and whether Microsoft or Apple recognizes that publisher. It does not look at what the program does.
- A program without the mark meets no check. A copy of ftorrent you build yourself from the source opens directly, and so does one you download with a command-line tool, which sets no mark.

So the prompt is about the file's origin and who vouches for it. Once you confirm it, the system remembers, and ftorrent opens like any other program from then on.

## Windows

`ftorrent.exe` runs on Windows 10 and 11, on 64-bit Intel and AMD processors.

### Installing and opening ftorrent on Windows

1. Download `ftorrent.exe`. Your browser may hold the download with a note that ftorrent.exe isn't commonly downloaded. In Microsoft Edge, open the **…** menu on the download, choose **Keep**, then **Show more**, then **Keep anyway**. Other browsers offer the same choice in a menu on the download itself.
2. Run `ftorrent.exe`. SmartScreen shows a blue window titled **Windows protected your PC**, saying **Microsoft Defender SmartScreen prevented an unrecognized app from starting. Running this app might put your PC at risk.** The only button is **Don't run**. Click **More info**, and the window adds the file's name, **Publisher: Unknown publisher**, and a **Run anyway** button. Click **Run anyway**.
3. The installer runs. It installs ftorrent for your Windows account alone, so it doesn't ask for an administrator password.
4. Open ftorrent from the Start menu. The installer wrote the program to your disk, so the installed copy carries no mark and opens without a prompt, now and later.

You can also clear the mark yourself before step 2, which skips the SmartScreen window. Right-click `ftorrent.exe`, choose **Properties**, and on the **General** tab find the line **This file came from another computer and might be blocked to help protect this computer**. Check **Unblock** beside it and click **OK**. In PowerShell, this command does the same:

```powershell
Unblock-File ~\Downloads\ftorrent.exe
```

On some Windows 11 computers, the message comes from **Smart App Control** instead, and has no **Run anyway** button and no **More info** link. Smart App Control blocks every program it can't vouch for, whether or not the program is marked, and it has no way to allow a single program. It has to be turned off, as the next section describes.

### Letting every program run on Windows

Both settings are in the **Windows Security** app, under **App & browser control**. Changing either one requires an administrator account.

- **Reputation-based protection settings**, then **Check apps and files**. Turning this off stops SmartScreen from checking programs downloaded from the web, so marked files run without the blue window. The **SmartScreen for Microsoft Edge** switch on the same page controls Edge's download warnings, the ones in step 1.
- **Smart App Control settings**, then **Off**. This setting exists only on Windows 11. On older builds, turning it off is permanent until Windows is reinstalled, while more recent updates allow it to be turned back on. The settings page states which applies to your computer, so read it before you choose.

Microsoft Defender Antivirus is a separate mechanism, and it keeps scanning files with both settings off. These two settings control whether Windows checks a program's publisher and reputation. Neither one controls the malware scan.

## macOS

`ftorrent.dmg` runs on Macs with Apple silicon, M1 and later, with macOS 15 Sequoia or later. There is no build for Intel Macs.

### Installing and opening ftorrent on macOS

1. Download `ftorrent.dmg` and open it. Drag ftorrent into the **Applications** folder, then eject the disk image.
2. Double-click ftorrent in **Applications**. macOS shows **"ftorrent" Not Opened**, saying **Apple could not verify "ftorrent" is free of malware that may harm your Mac or compromise your privacy**, with the buttons **Done** and **Move to Trash**. Click **Done**.
3. Open **System Settings**, choose **Privacy & Security**, and scroll down to the **Security** section. It says **"ftorrent" was blocked to protect your Mac**, with an **Open Anyway** button beside it. Click **Open Anyway**.
4. macOS asks once more, in a dialog with its own **Open Anyway** button, and then asks for your login password or Touch ID. Confirm, and ftorrent opens.
5. macOS keeps this decision, so later launches open ftorrent directly.

The **Open Anyway** button stays in System Settings for about an hour after step 2. If it's gone, double-click ftorrent again to bring it back. Older guides describe Control-clicking the app and choosing **Open**, which no longer works from macOS 15 Sequoia on. System Settings is the way through, as Apple's own page [Open a Mac app from an unknown developer](https://support.apple.com/guide/mac-help/open-a-mac-app-from-an-unknown-developer-mh40616/mac) describes.

You can also clear the mark yourself after step 1, which skips steps 2 through 4. In Terminal:

```bash
xattr -dr com.apple.quarantine /Applications/ftorrent.app
```

`xattr` edits a file's extended attributes, `-d` deletes the named one, and `-r` covers every file inside the app bundle. Without its quarantine attribute, the app opens with no dialog.

### Letting every program open on macOS

The setting is **Allow applications from**, in **System Settings**, under **Privacy & Security**, in the **Security** section. It offers **App Store** and **App Store & Known Developers**. A third choice, **Anywhere**, stops Gatekeeper from checking downloaded programs, and macOS lists it only after you run this command in Terminal:

```bash
sudo spctl --global-disable
```

Then choose **Anywhere** in the menu and confirm with your password. To restore the check, choose either of the other two, or run `sudo spctl --global-enable`.

XProtect, the malware scanner built into macOS, is a separate mechanism and keeps running with **Anywhere** chosen. The setting controls whether macOS requires a program's publisher to be registered with Apple. It does not control the malware scan.

## Installing from the command line

Instead of downloading through your browser and then working through the prompts above, you can get ftorrent with one command. The command also checks the hash for you before anything opens.

**Windows:** Click **Start**, type **PowerShell**, and press **Enter**. Paste the command below, and press **Enter**.

<DownloadCommand file="ftorrent.exe">

```powershell
cd ~\Downloads
curl.exe -fsSL -o ftorrent_setup.exe https://ftorrent.com/ftorrent.exe
if ((Get-FileHash ftorrent_setup.exe).Hash -eq '0000000000000000000000000000000000000000000000000000000000000000') { .\ftorrent_setup.exe } else { 'The hash does not match. ftorrent was not installed.' }
```

</DownloadCommand>

**macOS:** Click search in the upper right, and type **Terminal**. Paste the command below, and press **Return**.

<DownloadCommand file="ftorrent.dmg">

```bash
cd ~/Downloads && \
curl -fsSL -o ftorrent_setup.dmg https://ftorrent.com/ftorrent.dmg && \
echo "0000000000000000000000000000000000000000000000000000000000000000  ftorrent_setup.dmg" | shasum -a 256 -c && \
open ftorrent_setup.dmg
```

</DownloadCommand>

Both commands do the same three things:

- They save the installer in your **Downloads** folder as `ftorrent_setup.exe` or `ftorrent_setup.dmg`.
- They compute the file's SHA-256 hash and compare it with the hash written into the command, which this page reads from the sidecar published beside the installer. If the two differ, the command stops, and nothing opens.
- They open the file, the same as a double-click. On Windows the installer runs and installs ftorrent for your account, without asking for an administrator password. On macOS the disk image appears, and you drag ftorrent into **Applications**.

Neither system shows its first-run prompt, because `curl` sets no mark on what it downloads. Smart App Control on Windows 11 is the exception: it checks programs whether or not they carry the mark, so if it's on, you have to turn it off first. The command saves the installer under a name of its own, `ftorrent_setup`, because a copy you already downloaded through your browser carries the mark, and a file saved over it keeps the mark it had. The macOS command has two spaces between the hash and the filename, which is the format `shasum -c` reads, so keep both if you type it out by hand.

## Why ftorrent is not signed

Microsoft and Apple each run a program that removes the prompts above. On Windows, a developer signs with a certificate from a certificate authority or from Microsoft's own signing service, and SmartScreen builds a reputation for that certificate as its downloads add up. On macOS, a developer joins the Apple Developer Program, signs each release with a Developer ID certificate Apple issues, and uploads each release to Apple for notarization. Apple scans the release and returns a ticket that Gatekeeper checks at launch. These programs do real work. They tie a file to an accountable publisher, notarization scans for known malware, and revocation lets a vendor stop a malicious program on millions of machines at once. For most people, installing mostly from stores, they are a reasonable default.

They also make each release depend on an ongoing approval: an account in good standing, a certificate that can be revoked, and a scan that has to pass, all on the vendor's schedule and under terms the vendor can change. ftorrent is built so that no single party holds that kind of decision over it. The protocols are open standards, the tracker and DHT node are public infrastructure anyone can run, and the source is public for anyone to build. We keep the client consistent with the rest: ftorrent isn't registered with an app store or a developer program, and its releases don't go through signing or notarization.

This matters beyond our own copy. We want readers to build ftorrent from this repository and publish their own copies. To reach people as smoothly as a signed program does, each of those copies would need a paid registration of its own. Without one, a copy published from a fork is in exactly the position ftorrent is, and the instructions on this page work for it unchanged.

Here is what ftorrent offers instead, and what each part proves:

- **The source is public.** Anyone can read what the client does and build the copy they run. A copy you build yourself proves the most, and meets no prompt at all.
- **Each installer is published beside its SHA-256 hash, and the hash is committed to the public repository, dated.** A matching hash proves that your file is byte for byte the one we published. The copy on GitHub is a second record, in public history, that anyone tampering with the download would also have to change. A hash doesn't prove that a program is safe. That trust rests on the public source and on the project's record.
- **The Mac app carries an ad-hoc signature,** a seal with no publisher's name on it. It lets macOS confirm that the app bundle is intact, which is why macOS offers **Open Anyway** rather than reporting the app as damaged.

Both systems still leave the final decision with the person at the keyboard. **Run anyway** and **Open Anyway** are in plain sight, and this page shows where. The computer belongs to the person using it, and so does the choice of what runs on it.

## Checking the hash

The hash for each installer appears in its box at the top of this page. The page reads it from the installer's sidecar, a small JSON file published at the same address with `.json` added, such as `https://ftorrent.com/ftorrent.exe.json`, which also records the version, the architecture, the size in bytes, and the build date. The same sidecars are committed in the repository's [desktop/release](https://github.com/zootella/ftorrent/tree/master/desktop/release) folder, where git history dates each release.

On Windows, in PowerShell:

```powershell
Get-FileHash ~\Downloads\ftorrent.exe
```

On macOS, in Terminal:

```bash
shasum -a 256 ~/Downloads/ftorrent.dmg
```

Compare the output with the hash at the top of this page. PowerShell prints the hash in uppercase letters, which doesn't change its value. If the two match, your file is the one we published. If they don't, delete the file, don't run it, and download it again.
