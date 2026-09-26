<#
.\desktop\icon-studio\Add-Donut.ps1

Draw the donut onto every size of the sheet: sheet-<size>.png in, torrent-<size>.png out. The donut is donut.svg, circles on a sixteen unit grid. This reads each circle's center, radius, and color out of that file and draws it with GDI+ at each sheet's scale, sixteen grid units to the sheet's width, so donut.svg is the one place the shape is written, and a change there is a change at every size the next time this runs.

The sixteen grid is the point. The sheet's smallest size is 16 pixels, so a circle whose center and radius are whole units lands on whole pixels there, with a clean rim and nothing to hint by hand. The larger sizes are the same drawing scaled up, sharp wherever the scale is a whole number and gently antialiased where it isn't, which is what the sheet's own border does at those sizes too.

	.\Add-Donut.ps1
#>

param(
	[string]$Donut = 'donut.svg', #the overlay: <circle> elements on a 16 by 16 viewBox, drawn bottom to top in file order
	[string]$Sheet = 'sheet', #the blank pages, read as Sheet-<size>.png
	[string]$Torrent = 'torrent', #the result, written as Torrent-<size>.png
	[int[]]$Sizes = @(256, 64, 48, 40, 32, 24, 20, 16) #every layer the sheet has
)
Add-Type -AssemblyName System.Drawing #gdi+, the drawing library every windows has, reached through .net
Set-Location $PSScriptRoot #this folder, wherever the script was run from
$Sheet = Join-Path $PSScriptRoot $Sheet #full paths, because a .net call given a relative one resolves it against the process's working directory, which is not the folder powershell is sitting in
$Torrent = Join-Path $PSScriptRoot $Torrent

$circles = [regex]::Matches((Get-Content $Donut -Raw), '<circle cx="([\d.]+)" cy="([\d.]+)" r="([\d.]+)" fill="([^"]+)"/>') | ForEach-Object { #the four numbers and the color out of each <circle>, by pattern rather than by an xml parser, because the donut is written by hand in exactly this form
	@{cx = [double]$_.Groups[1].Value; cy = [double]$_.Groups[2].Value; r = [double]$_.Groups[3].Value; fill = $_.Groups[4].Value}
}
if ($circles.Count -eq 0) { throw "no <circle> in $Donut; the donut is circles and this draws nothing else" }

foreach ($size in $Sizes) {
	$page = [System.Drawing.Bitmap]::FromFile("$Sheet-$size.png")
	$bitmap = $page.Clone((New-Object System.Drawing.Rectangle -ArgumentList 0, 0, $size, $size), [System.Drawing.Imaging.PixelFormat]::Format32bppArgb) #the sheet's pixels copied bit for bit into a bitmap of our own; drawing the page onto a fresh image instead would send it through gdi+'s compositor, which resamples it by half a pixel and rounds the color of every soft-shadow pixel, and the loaded image itself is read only while its file is open
	$page.Dispose()
	$graphics = [System.Drawing.Graphics]::FromImage($bitmap) #the drawing surface over the bitmap
	$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias #the circles' edges blend into the page beneath them
	$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality #treat pixel coordinates as the corners of pixels, so a circle from 5 to 11 covers exactly pixels 5 through 10, edge to edge; the default treats them as centers and would smear every edge across two pixels
	$scale = $size / 16 #one grid unit in pixels at this size
	foreach ($circle in $circles) {
		$brush = New-Object System.Drawing.SolidBrush -ArgumentList ([System.Drawing.ColorTranslator]::FromHtml($circle.fill)) #a fill color from the svg's #rrggbb or a named color like white
		$graphics.FillEllipse($brush, [single](($circle.cx - $circle.r) * $scale), [single](($circle.cy - $circle.r) * $scale), [single](2 * $circle.r * $scale), [single](2 * $circle.r * $scale)) #gdi+ takes the box around the circle, its top left and its size, not the center and radius the svg gives
		$brush.Dispose()
	}
	$graphics.Dispose()
	$bitmap.Save("$Torrent-$size.png", [System.Drawing.Imaging.ImageFormat]::Png)
	$bitmap.Dispose()
	"wrote    $(Split-Path $Torrent -Leaf)-$size.png"
}
