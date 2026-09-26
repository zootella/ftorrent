_ftorrent/desktop/icon-studio/README.md_

# The icon studio

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5
> <br>Created: 2026-Sep
> <br>Last reviewed: 2026-Sep
> <br>Windows PowerShell: 5.1
> <br>.NET Framework, System.Drawing: 4.8
> <br>Tauri CLI, for `tauri icon`: 2.11

Where ftorrent's icons are made, and the map of where they go. The first half of this document is for anyone working in the monorepo who needs to know what an icon file is, where it comes from, and which build reads it. The second half is for whoever opens this folder to change a drawing.

## Icons across the monorepo

ftorrent has three icons, all drawn from two marks in one brand color, `#FF7900`:

- **The application icon**, the brand: an orange pill holding two white rings joined by a bar. Two nodes, linked. It is the icon of the desktop client on every platform, of the Windows installer, and of the websites.
- **The document icon**, the donut on the sheet: a blank page with a folded corner carrying one orange circle with a white dot. One node, waiting to join. It is what a `.torrent` file wears in Explorer, and it exists only on Windows.
- **The favicon**, the brand again, as an SVG the websites link inline.

Everything starts from three files in this folder and ends as three files, with the intermediate files kept in between so any step can be looked at. The studio is a handful of PowerShell scripts, run by hand on Windows only; one command, `.\Update-Icons.ps1`, runs the whole chain.

| in | intermediate, in this folder | intermediate, in `src-tauri/icons/` | out |
|---|---|---|---|
| `brand.svg`, the mark | | `app-icon.svg`, `app-icon-mac.svg`, `app-icon-tile.svg`, then everything `tauri icon` generates from them | `favicon.svg`, `ftorrent.ico` |
| `donut.svg`, the node | `torrent-256.png`, `-64`, `-48`, `-40`, `-32`, `-24`, `-20`, `-16` | | `torrent.ico` |
| `sheet.ico`, the blank page | `sheet-256.png`, `-64`, `-48`, `-40`, `-32`, `-24`, `-20`, `-16` | | |

The outputs land here as the record of what shipped, and each has one place it is copied to. `torrent.ico` and the three `app-icon*.svg` are copied by the command; `favicon.svg` is copied by hand.

### The application icon, into the desktop build

Tauri takes its icons from `desktop/src-tauri/icons/`, listed in `tauri.conf.json` under `bundle.icon`, and picks one per platform by file extension:

```
icons/icon.ico              Windows: the executable's icon, which the taskbar, Explorer, and the tray show
icons/mac/icon.icns         macOS: the bundle's icon, which the Dock, Finder, and the disk image show
icons/32x32.png             Linux: the sizes the .deb and .rpm install into the icon theme, and the Flatpak renames to its id
icons/128x128.png
icons/128x128@2x.png
```

None of those is drawn. They are generated from `brand.svg` by three layers of tooling, each doing one thing:

1. **The studio writes the sources.** `Update-Icons.ps1` reads `brand.svg` and writes it into `src-tauri/icons/` three times as `app-icon.svg`, `app-icon-mac.svg`, and `app-icon-tile.svg`, changing only the viewBox. A wider viewBox shows more empty space around the same drawing, so a generator that renders it edge to edge produces an inset mark. `app-icon.svg` keeps `0 0 16 16`, full bleed, which is what Windows and Linux want: their icons fill the canvas, and Windows treats a margin as a defect. `app-icon-mac.svg` widens to `-1.184 -1.184 18.368 18.368`, so the mark takes 892 of the 1024 canvas, because macOS draws an icon exactly as authored on a grid where every neighbor leaves a margin, and a bare shape sized against Apple's own round icons lands at 892. `app-icon-tile.svg` widens to `-4.118 -4.118 24.237 24.237`, 676 of 1024, for the Windows 10 Start menu tile, where the mark sits inset on the tile's background. The workspace README has the measurements behind those two numbers.
2. **Our build pipeline routes the three sources so they can't overwrite each other.** The `icons` script in `package.json`, `pnpm icons`, runs Tauri's generator three times: the full-bleed source into `src-tauri/icons/` itself, the Mac source into a gitignored scratch folder `.mac/`, and the tile source into `.tile/`. Then `node scripts.js icons-collect`, ours, copies the few wanted files out from under the generator's names: `.mac/icon.icns` to `icons/mac/icon.icns`, and two Store logos from `.tile/` to `icons/tile/tile-medium.png` and `tile-small.png`. The full-bleed run also wrote an `icons/icon.icns` of its own, which nothing uses.
3. **Tauri's tool renders.** `tauri icon`, from the Tauri CLI, takes one square SVG and resizes it edge to edge into every output at once: `icon.ico`, `icon.icns`, the Linux PNGs, the Store logos, and mobile trees. It has no notion of a safe area and never will; the margin has to be in the source, which is why step 1 exists.

Then `tauri.conf.json` picks. `bundle.icon` names `icons/icon.ico` and the three PNGs from the full-bleed run, and `icons/mac/icon.icns` from the Mac run rather than the full-bleed `.icns` beside it. Tauri chooses the `.icns` for the Mac by extension, so pointing that one entry at the inset file is the whole per-platform switch. `bundle.resources` lands the two tile PNGs beside the executable with `ftorrent.VisualElementsManifest.xml`, the file Windows 10 reads to draw a large tile.

The Windows installer and uninstaller wear the same `.ico`, through two more lines in `tauri.conf.json`, `bundle.windows.nsis.installerIcon` and `uninstallerIcon`, both pointing at `icons/icon.ico`. Without them the setup exe shows NSIS's own stock icon, and the first piece of ftorrent anyone meets is a grey box in someone else's house style. The studio keeps a copy of that `.ico` as `ftorrent.ico`, so the three outputs sit together.

The engine is the one executable a Windows user may meet that keeps a stock icon: `ftorrent-engine.exe`, the frozen Python holding libtorrent, appears in Task Manager and in the firewall's first prompt wearing PyInstaller's default. That is a choice, not an omission; the icon suits the era the client is drawn from, and changing it would be one `icon=` argument in `engine/ftorrent-engine.spec`.

### The document icon, into the Windows build

`torrent.ico` is copied to `desktop/src-tauri/icons/torrent.ico`, and `bundle.resources` in `tauri.conf.json` lands it beside the executable, where the Windows file type registration names it as the `.torrent` type's `DefaultIcon`. It is Windows only. macOS composes a document icon itself, a page with the application's icon set on it, for any type an app declares; Linux draws the desktop theme's own icon for the file's MIME type. Both look right with nothing supplied, so nothing is.

### The favicon, into the websites

`favicon.svg` is `brand.svg` unchanged, and it is copied by hand into each site's public folder, `site/public/` for ftorrent.com and `docs/docs/public/` for docs.ftorrent.com, where each site's config links it: `<link rel="icon" type="image/svg+xml" href="/favicon.svg">`. An SVG favicon scales to every tab and pinned-site size, and is what current Chrome, Firefox, and Edge read; Safari still wants a raster, so `ftorrent.ico` goes beside it as `favicon.ico`. open.ftorrent.com is the exception and stays one: its favicon is an earth emoji, which is right for a page whose whole subject is the planet's peers.

## Inside the studio

This folder is Windows only and by hand. Nothing in the build reads it, and Tauri never looks in it, the way it never looks in `linux/`. The scripts are PowerShell, drawing with the `System.Drawing` that every Windows has, and they run a few times a year, when a drawing changes. Each script opens with an essay saying what it does and how the format it touches works, so the scripts are the reference and this section is the tour.

### The drawings

**`brand.svg`** is the mark on a sixteen unit grid: `viewBox="0 0 16 16"` with `width="1024"`, so a viewer opens it large while every coordinate stays a small number that lands on a pixel at the smallest sizes. Two elements, a rounded rectangle and one path with two arcs. The designer's first draft was six overlapping shapes; this is that draft simplified, and it renders pixel-identical through the same rasterizer.

**`donut.svg`** is the node on the same grid: an orange circle of diameter 8 centered at (8,9), and a white dot of diameter 2 on the same center. Whole units for the center and both radii, so at 16 pixels every edge sits on a pixel, with a clean one-pixel rim and nothing to hint by hand. The larger sizes are the same drawing scaled, sharp where the scale is whole and gently soft where it isn't, which is what the sheet's own border does at those sizes too.

**`sheet.ico`** is the blank page as it arrived: drawn by Fuji's designer for that project and lent to this one, eight layers from 256 down to 16, including the 20 and 40 that Windows wants at 125 percent scaling, the 256 stored as a PNG and the rest as bare Windows bitmaps. `sheet-<size>.png` are those eight layers, split out once.

### The scripts

- **`Update-Icons.ps1`** is the one command. It runs `Add-Donut`, runs `Join-Ico` and copies the result into `src-tauri/icons/`, writes `favicon.svg` and the three `app-icon*.svg` from `brand.svg` by swapping the viewBox, runs `pnpm icons` in the workspace above, and copies the `icon.ico` that produced back here as `ftorrent.ico`. Of those, only the viewBox writing and the two copies are the studio's own work on the application icon; the generating is Tauri's and the routing is the workspace's, as the first half of this document lays out.
- **`Add-Donut.ps1`** draws the donut onto every sheet size: `sheet-<size>.png` in, `torrent-<size>.png` out. It reads each circle's center, radius, and color out of `donut.svg` by pattern and draws it with GDI+ at sixteen grid units to the sheet's width, so `donut.svg` is the only place the shape is written.
- **`Join-Ico.ps1`** packs `torrent-<size>.png` into `torrent.ico`: a six byte header, a sixteen byte directory entry per layer, then the layers, each stored as a PNG, largest first.
- **`Split-Ico.ps1`** is the reverse, an `.ico` into one PNG per layer, converting a layer stored as a bare bitmap along the way. It made the eight `sheet-<size>.png` and runs again only when a new sheet arrives.
- **`Test-Ico.ps1`** asks the Windows shell for every layer of an icon, the way Explorer does, and reports the size and solid pixel count of each. It is the check to run on a packed icon.

### Changing the donut

Edit `donut.svg`, keeping the center and radii on whole units, run `.\Update-Icons.ps1`, and look at `torrent.ico` in Explorer: switching the folder's view between Details, Small, Medium, Large, and Extra large shows each layer in turn, beside whatever real files are around. To see it on a real `.torrent` file, install a build that registers the type. Commit the changed drawing, the eight `torrent-<size>.png`, both copies of `torrent.ico`, and nothing in `src-tauri/icons/` will have changed but that one file.

### Changing the brand

Edit `brand.svg` and run the same command. This time the three `app-icon*.svg` and everything `pnpm icons` generates change too, which is a large diff of generated files, and it is meant to be committed whole: the generated icons are artifacts frozen at the CLI that made them, and a CLI fix reaches them only when someone re-runs the generator, so run this after a Tauri CLI upgrade as well. Expect `icon.icns` and `mac/icon.icns` to show as modified after every run whether or not the artwork changed; the CLI writes their entries in an unstable order, and the image bytes are identical. The generator also writes `android/` and `ios/` trees nothing here uses.

### Checking a packed icon

Use the shell, not .NET. `System.Drawing.Icon`, .NET's own icon class, refuses PNG layers it did not write itself, while `SHDefExtractIcon`, which Explorer uses, reads every one; `Test-Ico.ps1` calls the latter. Each layer of a packed sheet came out of the shell identical to the drawn original when this was checked.

### Five traps, met and written into the scripts

Three are PowerShell's. A byte array sent down a pipeline is unrolled into single bytes, so the layers travel inside objects. A shifted `[byte]` stays a byte, so 256 wraps to 0; the scripts read multi-byte numbers with `BitConverter` instead. A .NET call given a relative path resolves it against the process's working directory, not the folder PowerShell is sitting in, so every path handed to .NET is made absolute first. And PowerShell 5.1's `-Encoding utf8` writes a byte order mark, three invisible bytes at the front of the file, so the SVGs are written through .NET with an encoding that doesn't.

One is GDI+'s. Drawing an image onto another at the same size still runs it through the default bilinear filter, which shifts it by half a pixel and blurs a 16 pixel page's edge; the copy is made with nearest-neighbor interpolation and a half-pixel offset, which lands every pixel on itself.
