_ftorrent/desktop/icon-studio/README.md_

# The icon studio

> Prepared by [Claude Code](https://claude.ai/code) using Fable 5
> <br>Created: 2026-Sep
> <br>Last reviewed: 2026-Oct
> <br>Windows PowerShell: 5.1
> <br>.NET Framework, System.Drawing: 4.8
> <br>Tauri CLI, for `tauri icon`: 2.11

Where ftorrent's icons are made, and the map of where they go. The first half of this document is for anyone working in the monorepo who needs to know what an icon file is, where it comes from, and which build reads it. The second half is for whoever opens this folder to change a drawing.

## Icons across the monorepo

ftorrent has four icons, all drawn from two marks, and all but one in the brand color, `#FF7900`:

- **The application icon**, the brand: an orange pill holding two white rings joined by a bar. Two nodes, linked. It is the icon of the desktop client on every platform, of the Windows installer, and of the websites.
- **The document icon**, the donut on the sheet: a blank page with a folded corner carrying one orange circle with a white dot. One node, waiting to join. It is what a `.torrent` file wears in Explorer, and it exists only on Windows.
- **The tray icon**, the glyph: the pill as a stencil, with the two nodes and their bar cut out of it, filled in one color. It is what ftorrent shows in the Windows notification area while it runs, white on a dark taskbar and black on a light one, and it is the drawing the Mac's menu bar icon is rendered from too. It is the one place the brand is not orange, on purpose: Windows programs put their full-color icon in the tray, and ftorrent's reads as a glyph beside the system's own.
- **The favicon**, the brand again, as an SVG the websites link inline.

Everything starts from four files in this folder and ends as five files, with the intermediate files kept in between so any step can be looked at. The studio is a handful of PowerShell scripts, run by hand on Windows only; one command, `.\Update-Icons.ps1`, runs the whole chain.

| in | intermediate, in this folder | intermediate, in `src-tauri/icons/` | out |
|---|---|---|---|
| `brand.svg`, the mark | | `app-icon.svg`, `app-icon-mac.svg`, `app-icon-tile.svg`, then everything `tauri icon` generates from them | `favicon.svg`, `ftorrent.ico` |
| `glyph.svg`, the stencil | `tray-white-64.png`, `-48`, `-40`, `-32`, `-24`, `-20`, `-16`, and the same seven in black | | `tray-white.ico`, `tray-black.ico` |
| `donut.svg`, the node | `torrent-256.png`, `-64`, `-48`, `-40`, `-32`, `-24`, `-20`, `-16` | | `torrent.ico` |
| `sheet.ico`, the blank page | `sheet-256.png`, `-64`, `-48`, `-40`, `-32`, `-24`, `-20`, `-16` | | |

The outputs land here as the record of what shipped. The command copies `torrent.ico`, the two tray icons, and the three `app-icon*.svg`; `favicon.svg` lives inline in the two branded sites' configs.

### The application icon, into the desktop build

Tauri takes its icons from `desktop/src-tauri/icons/`, listed in `tauri.conf.json` under `bundle.icon`, and picks one per platform by file extension:

```
icons/icon.ico              Windows: the executable's icon, which the taskbar and Explorer show
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

The Windows installer and uninstaller wear the same `.ico`, through two more lines in `tauri.conf.json`, `bundle.windows.nsis.installerIcon` and `uninstallerIcon`, both pointing at `icons/icon.ico`. Without them the setup exe shows NSIS's own stock icon, and the first piece of ftorrent anyone meets is a grey box in someone else's house style. The studio keeps a copy of that `.ico` as `ftorrent.ico`, so the outputs sit together.

The engine is the one executable a Windows user may meet that keeps a stock icon: `ftorrent-engine.exe`, the frozen Python holding libtorrent, appears in Task Manager and in the firewall's first prompt wearing PyInstaller's default. That is a choice, not an omission; the icon suits the era the client is drawn from, and changing it would be one `icon=` argument in `engine/ftorrent-engine.spec`.

### The document icon, into the Windows build

`torrent.ico` is copied to `desktop/src-tauri/icons/torrent.ico`, and `bundle.resources` in `tauri.conf.json` lands it beside the executable, where the Windows file type registration names it as the `.torrent` type's `DefaultIcon`. It is Windows only. macOS composes a document icon itself, a page with the application's icon set on it, for any type an app declares; Linux draws the desktop theme's own icon for the file's MIME type. Both look right with nothing supplied, so nothing is.

### The tray icon, into the Windows build

`tray-white.ico` and `tray-black.ico` are copied to `desktop/src-tauri/icons/`, and `lifecycle.rs` in `src-tauri` compiles both into the program with `include_bytes!`, so neither can go missing from an installed copy and neither needs a line in `tauri.conf.json`. Each has seven layers, the taskbar's scale ladder: 16 pixels at 100 percent scaling, 20 at 125, 24 at 150, 32 at 200, 40 at 250, 48 at 300, and 64 at 400, and no 256, since nothing draws a tray icon that large. At startup, Rust asks Windows for its small icon size, which is 16 scaled by the system's setting, and hands the shell the layer of exactly that size, so the icon is never resized on the way to the screen. That is the reason the tray has files of its own rather than the application icon: Tauri hands a tray the first layer of `icon.ico`, which is 32 pixels, and the shell shrinks it, softening a mark drawn to land on pixels at 16.

Two files because the notification area paints an icon's pixels exactly as given, where the Mac's menu bar takes a template image and tints its shape itself. So the app carries the glyph in both colors and chooses: the taskbar's theme is one registry value, `SystemUsesLightTheme` under the user's Personalize key, separate from the apps' theme the window follows, and Windows 10 ships with the taskbar dark and apps light while Windows 11 ships with both light. A 1 there means a light taskbar and the black glyph; anything else, including the value's absence on older Windows 10 builds, means the white one. A thread in the Rust core waits on that key for the life of the process, so changing the theme in Settings while ftorrent runs swaps the icon at once.

### The menu bar icon, on the Mac

The same `glyph.svg` is the source for the Mac's menu bar icon, rendered there to a PNG at 16 points and 32 for Retina, in any one color. The Mac marks the image as a template when it hands it to the system, and macOS then reads only the shape, the opaque pixels, and paints it in the menu bar's own color, black on a light bar and white on a dark one. The studio's scripts are Windows only, so that rendering is done on the Mac.

`favicon.svg` is `brand.svg` unchanged, and the heads of ftorrent.com and docs.ftorrent.com carry it inline as a data URI, set in `site/nuxt.config.ts` and `docs/docs/.vitepress/config.js`, with its `#` written `%23` and its double quotes made single. Inline, a site carries no favicon file at all, and an SVG favicon scales to every tab and pinned-site size. open.ftorrent.com and good.ftorrent.com are the exception: their favicon is an earth emoji, inline the same way, which is right for pages whose whole subject is the planet's peers.

## Inside the studio

This folder is Windows only and by hand. Nothing in the build reads it, and Tauri never looks in it, the way it never looks in `linux/`. The scripts are PowerShell, drawing with the `System.Drawing` that every Windows has, and they run a few times a year, when a drawing changes. Each script opens with an essay saying what it does and how the format it touches works, so the scripts are the reference and this section is the tour.

### The drawings

**`brand.svg`** is the mark on a sixteen unit grid: `viewBox="0 0 16 16"` with `width="1024"`, so a viewer opens it large while every coordinate stays a small number that lands on a pixel at the smallest sizes. Two elements, a rounded rectangle and one path with two arcs. The designer's first draft was six overlapping shapes; this is that draft simplified, and it renders pixel-identical through the same rasterizer.

**`donut.svg`** is the node on the same grid: an orange circle of diameter 8 centered at (8,9), and a white dot of diameter 2 on the same center. Whole units for the center and both radii, so at 16 pixels every edge sits on a pixel, with a clean one-pixel rim and nothing to hint by hand. The larger sizes are the same drawing scaled, sharp where the scale is whole and gently soft where it isn't, which is what the sheet's own border does at those sizes too.

**`glyph.svg`** is the mark as a stencil, on the same grid: one path holding two closed figures, the pill and the nodes with their bar, with `fill-rule="evenodd"`, which leaves the inner figure as a hole through the outer one. It is drawn by hand from the same numbers as `brand.svg`, the pill's rectangle written as a path so the two figures can share one `d`, and a change to the brand's shape is a change here too. Its fill color is a placeholder: the renderer fills it in each of the tray's colors, and the Mac's template rendering ignores color altogether.

**`sheet.ico`** is the blank page as it arrived: drawn by a designer for another project and lent to this one, eight layers from 256 down to 16, including the 20 and 40 that Windows wants at 125 percent scaling, the 256 stored as a PNG and the rest as bare Windows bitmaps. `sheet-<size>.png` are those eight layers, split out once.

### The scripts

- **`Update-Icons.ps1`** is the one command. It runs `Add-Donut`, runs `Join-Ico` and copies the result into `src-tauri/icons/`, writes `favicon.svg` and the three `app-icon*.svg` from `brand.svg` by swapping the viewBox, runs `Draw-Tray` on `glyph.svg` and `Join-Ico` on each color's set and copies the two results into `src-tauri/icons/` too, runs `pnpm icons` in the workspace above, and copies the `icon.ico` that produced back here as `ftorrent.ico`. Of those, only the viewBox writing and the two copies are the studio's own work on the application icon; the generating is Tauri's and the routing is the workspace's, as the first half of this document lays out.
- **`Add-Donut.ps1`** draws the donut onto every sheet size: `sheet-<size>.png` in, `torrent-<size>.png` out. It reads each circle's center, radius, and color out of `donut.svg` by pattern and draws it with GDI+ at sixteen grid units to the sheet's width, so `donut.svg` is the only place the shape is written.
- **`Draw-Tray.ps1`** draws the glyph at every taskbar scale in each of the tray's two colors: `glyph.svg` in, `tray-white-<size>.png` and `tray-black-<size>.png` out, on a clear ground. It reads the path with a small interpreter for the commands the glyph uses, turning each SVG arc into the center, start angle, and sweep GDI+ draws from, and fills the path under GDI+'s even-odd rule, so the hole's rim is antialiased correctly; painting the hole afterward in transparent would leave that rim half dark.
- **`Join-Ico.ps1`** packs `torrent-<size>.png` into `torrent.ico`, and each color's `tray-<color>-<size>.png` into `tray-<color>.ico`: a six byte header, a sixteen byte directory entry per layer, then the layers, each stored as a PNG, largest first.
- **`Split-Ico.ps1`** is the reverse, an `.ico` into one PNG per layer, converting a layer stored as a bare bitmap along the way. It made the eight `sheet-<size>.png` and runs again only when a new sheet arrives.
- **`Test-Ico.ps1`** asks the Windows shell for every layer of an icon, the way Explorer does, and reports the size and solid pixel count of each. It is the check to run on a packed icon.

### Changing the donut

Edit `donut.svg`, keeping the center and radii on whole units, run `.\Update-Icons.ps1`, and look at `torrent.ico` in Explorer: switching the folder's view between Details, Small, Medium, Large, and Extra large shows each layer in turn, beside whatever real files are around. To see it on a real `.torrent` file, install a build that registers the type. Commit the changed drawing, the eight `torrent-<size>.png`, both copies of `torrent.ico`, and nothing in `src-tauri/icons/` will have changed but that one file.

### Changing the brand

Edit `brand.svg`, carry the same change into `glyph.svg` by hand, and run the same command. This time the fourteen `tray-<color>-<size>.png`, both copies of each tray icon, the three `app-icon*.svg`, and everything `pnpm icons` generates change too, which is a large diff of generated files, and it is meant to be committed whole: the generated icons are artifacts frozen at the CLI that made them, and a CLI fix reaches them only when someone re-runs the generator, so run this after a Tauri CLI upgrade as well. Expect `icon.icns` and `mac/icon.icns` to show as modified after every run whether or not the artwork changed; the CLI writes their entries in an unstable order, and the image bytes are identical. The generator also writes `android/` and `ios/` trees nothing here uses.

### Changing the glyph

Edit `glyph.svg`, keeping it one path of closed figures in the commands the renderer reads, absolute M, L, H, V, A, and Z, and run the same command. Explorer is a poor place to look at the result, since it shows an icon on white, where the white glyph vanishes; run the app instead and watch the tray while switching the taskbar's theme in Settings, under Personalization, Colors, with Custom chosen so the taskbar and the apps can be set apart. `Test-Ico.ps1` counts each layer's solid pixels, and the two colors should count the same. Commit the drawing, the fourteen `tray-<color>-<size>.png`, and both copies of each `.ico`.

### Checking a packed icon

Use the shell, not .NET. `System.Drawing.Icon`, .NET's own icon class, refuses PNG layers it did not write itself, while `SHDefExtractIcon`, which Explorer uses, reads every one; `Test-Ico.ps1` calls the latter. Each layer of a packed sheet came out of the shell identical to the drawn original when this was checked.

### Five traps, met and written into the scripts

Three are PowerShell's. A byte array sent down a pipeline is unrolled into single bytes, so the layers travel inside objects. A shifted `[byte]` stays a byte, so 256 wraps to 0; the scripts read multi-byte numbers with `BitConverter` instead. A .NET call given a relative path resolves it against the process's working directory, not the folder PowerShell is sitting in, so every path handed to .NET is made absolute first. And PowerShell 5.1's `-Encoding utf8` writes a byte order mark, three invisible bytes at the front of the file, so the SVGs are written through .NET with an encoding that doesn't.

One is GDI+'s. Drawing an image onto another at the same size still runs it through the default bilinear filter, which shifts it by half a pixel and blurs a 16 pixel page's edge; the copy is made with nearest-neighbor interpolation and a half-pixel offset, which lands every pixel on itself.
