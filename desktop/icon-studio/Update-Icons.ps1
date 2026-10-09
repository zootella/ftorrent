<#
.\desktop\icon-studio\Update-Icons.ps1

The one command: from brand.svg, donut.svg, glyph.svg, and the sheet, make favicon.svg, ftorrent.ico, torrent.ico, tray-white.ico, and tray-black.ico, and put copies where the build and the websites read them. Run it after changing a drawing, then look at the results in Explorer.

What it does, in order:
	1. Add-Donut.ps1 draws the donut onto every sheet size, sheet-<size>.png to torrent-<size>.png.
	2. Join-Ico.ps1 packs those into torrent.ico, and a copy goes to src-tauri\icons\torrent.ico, which bundle.resources lands beside the executable for the .torrent file type's icon.
	3. brand.svg is written out as favicon.svg, the same drawing, for the websites to link inline; and as four files in src-tauri\icons that differ only in viewBox: app-icon.svg, app-icon-mac.svg, and app-icon-tile.svg, the sources the application icon pipeline reads, the same drawing seen full bleed, inset for the macOS Dock, and inset for the Windows Start menu tile; and app-icon-wide.svg, cropped to the pill, which the About page shows.
	4. Draw-Tray.ps1 renders glyph.svg, the mark as a stencil with the nodes cut out, at every size the notification area draws at and in each of the tray's two colors, tray-white-<size>.png and tray-black-<size>.png, and Join-Ico.ps1 packs each set into its .ico, which go to src-tauri\icons for lifecycle.rs to compile into the program and choose between by the taskbar's theme.
	5. pnpm icons, in the workspace above, generates every platform's application icon from the three app-icon sources. That is Tauri's own tool, and the only step here that isn't ours.
	6. The .ico it wrote, src-tauri\icons\icon.ico, comes back here as ftorrent.ico, the Windows application and installer icon, kept beside the other outputs as the record of what shipped.

The sheet's layers, sheet-<size>.png, are not remade here; Split-Ico.ps1 makes them once when a new sheet.ico arrives.

	.\Update-Icons.ps1
#>

$ErrorActionPreference = 'Stop' #any error ends the run, rather than powershell's default of printing it and carrying on
Set-Location $PSScriptRoot #this folder, wherever the script was run from
$icons = Join-Path $PSScriptRoot '..\src-tauri\icons' #where the build reads its icons

#the insets the application icon pipeline needs, as viewBoxes over the brand's sixteen unit grid. The brand fills its canvas edge to edge, which is right on Windows and Linux. macOS draws an icon exactly as authored, on a grid where a squircle fills 824 of 1024, and a bare shape sized against Apple's own round icons lands at 892, so that viewBox widens the view until the drawing takes 892 of the canvas. The Windows 10 Start menu tile wants the mark inset on the tile's background, at 676 of 1024. A wider viewBox shows more space around the same drawing, which is how one file becomes three without redrawing anything
$viewBoxes = [ordered]@{
	'app-icon.svg'      = '0 0 16 16'
	'app-icon-mac.svg'  = '-1.184 -1.184 18.368 18.368'
	'app-icon-tile.svg' = '-4.118 -4.118 24.237 24.237'
	'app-icon-wide.svg' = '0 4 16 8' #the pill's own bounds, the one window that isn't square, so the page can show the mark with no clear space above or below it; the generator never reads this one
}

"donut onto the sheets"
& .\Add-Donut.ps1 #the & runs a script by path; without it powershell would take the path as a string to print
"packing torrent.ico"
& .\Join-Ico.ps1 -Stem torrent -Ico torrent.ico
Copy-Item torrent.ico (Join-Path $icons 'torrent.ico')
"copied   torrent.ico to src-tauri\icons"

$brand = Get-Content brand.svg -Raw #-Raw reads the file as one string rather than as an array of lines
$utf8 = New-Object Text.UTF8Encoding $false #plain utf-8, no byte order mark; powershell's own -Encoding utf8 puts three invisible bytes at the front of a file, which browsers and generators tolerate and a diff against brand.svg does not
[IO.File]::WriteAllText((Join-Path $PSScriptRoot 'favicon.svg'), $brand, $utf8) #the brand as it is
"wrote    favicon.svg"
foreach ($name in $viewBoxes.Keys) {
	$box = $viewBoxes[$name] -split ' ' #the window's x, y, width, and height
	$svg = $brand -replace 'viewBox="[^"]*"', ('viewBox="' + $viewBoxes[$name] + '"') #the same drawing with its viewBox swapped
	$svg = ([regex]'height="[^"]*"').Replace($svg, ('height="' + [int](1024 * $box[3] / $box[2]) + '"'), 1) #and the file's height following the window's shape, so the square windows keep 1024 by 1024 and the wide one is 1024 by 512; without this an img would letterbox the wide view back into a square. The first match only, which is the root's: -replace would change every height in the file, the pill's rect among them
	[IO.File]::WriteAllText((Join-Path $icons $name), $svg, $utf8)
	"wrote    $name to src-tauri\icons"
}

"glyph at every taskbar scale, in white and in black"
& .\Draw-Tray.ps1
foreach ($color in 'white', 'black') { #one icon per color: the notification area paints nothing, so the app carries both and shows the one the taskbar's theme calls for
	"packing tray-$color.ico"
	& .\Join-Ico.ps1 -Stem "tray-$color" -Ico "tray-$color.ico"
	Copy-Item "tray-$color.ico" (Join-Path $icons "tray-$color.ico")
	"copied   tray-$color.ico to src-tauri\icons"
}

"pnpm icons, in the workspace above"
Push-Location .. #step up for the run and come back after, whatever happens
try { pnpm icons; if ($LASTEXITCODE -ne 0) { throw "pnpm icons failed with exit code $LASTEXITCODE" } } finally { Pop-Location } #a program's failure doesn't stop powershell on its own, so its exit code is checked by hand
Copy-Item (Join-Path $icons 'icon.ico') ftorrent.ico
"copied   ftorrent.ico back from src-tauri\icons"
"done: favicon.svg, ftorrent.ico, torrent.ico, tray-white.ico, tray-black.ico"
