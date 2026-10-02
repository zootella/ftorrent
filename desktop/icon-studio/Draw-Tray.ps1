<#
.\desktop\icon-studio\Draw-Tray.ps1

Draw the tray glyph at every size the Windows notification area uses, in each of the tray's colors: glyph.svg in, tray-white-<size>.png and tray-black-<size>.png out. The glyph is one path on a sixteen unit grid, the pill with the two nodes and their bar cut out of it, and this reads that path out of the file and draws it with GDI+ at each size's scale, sixteen grid units to the image's width, so glyph.svg is the one place the shape is written. The colors are this script's: white is for a dark taskbar and black for a light one, and lifecycle.rs in src-tauri picks between them by the taskbar's theme.

The sizes are the taskbar's scale ladder: 16 pixels at 100 percent scaling, 20 at 125, 24 at 150, 32 at 200, 40 at 250, 48 at 300, and 64 at 400. The shell draws a notification area icon at exactly one of these, and lifecycle.rs picks the matching layer, so none of them is ever resized on the way to the screen. There is no 256: nothing draws a tray icon that large.

The path is read by a small interpreter rather than by pattern, because a path with a hole in it is written as two closed figures in one d attribute, and the hole is made by the fill rule: evenodd leaves unfilled whatever an even number of figures cover. GDI+ has the same rule, Alternate, and fills the hole's rim correctly, where painting the hole afterward in transparent would leave its antialiased edge half dark. The interpreter knows the commands the glyph uses, absolute M, L, H, V, A, and Z, and an arc is turned into GDI+'s center, start angle, and sweep from the endpoints and flags SVG gives, by the arithmetic in the SVG specification's implementation notes.

	.\Draw-Tray.ps1
#>

param(
	[string]$Glyph = 'glyph.svg', #the drawing: one <path> with its d and fill-rule on a 16 by 16 viewBox
	[string]$Stem = 'tray', #the result, written as Stem-<color>-<size>.png
	[hashtable]$Colors = @{white = 'white'; black = 'black'}, #each name is a file's, and each value the color the glyph is filled with for it
	[int[]]$Sizes = @(64, 48, 40, 32, 24, 20, 16) #every size the notification area draws at, by scaling
)
Add-Type -AssemblyName System.Drawing #gdi+, the drawing library every windows has, reached through .net
Set-Location $PSScriptRoot #this folder, wherever the script was run from
$Stem = Join-Path $PSScriptRoot $Stem #a full path, because a .net call given a relative one resolves it against the process's working directory, which is not the folder powershell is sitting in

$svg = Get-Content $Glyph -Raw #-Raw reads the file as one string rather than as an array of lines
$element = [regex]::Match($svg, '<path d="([^"]+)"(?: fill-rule="([^"]+)")?') #the path's d, and its fill rule if it names one
if (-not $element.Success) { throw "no <path> in $Glyph; the glyph is one path and this draws nothing else" }
$d = $element.Groups[1].Value
$evenodd = $element.Groups[2].Value -eq 'evenodd' #svg's default rule is nonzero, which fills a figure inside another; evenodd leaves it as a hole

$tokens = [regex]::Matches($d, '[MLHVAZ]|-?[\d.]+') | ForEach-Object { $_.Value } #the path as a list of command letters and numbers, whatever spaces or commas sat between them
function Read-Number { $script:index++; [double]$tokens[$script:index - 1] } #the next token as a number, advancing

function Build-Path([double]$scale) { #the glyph as a GraphicsPath at one size, built fresh for each, since a path's points are in pixels
	$path = New-Object System.Drawing.Drawing2D.GraphicsPath
	if ($evenodd) { $path.FillMode = [System.Drawing.Drawing2D.FillMode]::Alternate } else { $path.FillMode = [System.Drawing.Drawing2D.FillMode]::Winding }
	$script:index = 0
	$x = 0.0; $y = 0.0 #the current point, in grid units
	$command = ''
	while ($script:index -lt $tokens.Count) {
		$token = $tokens[$script:index]
		if ($token -match '^[MLHVAZ]$') { $command = $token; $script:index++ } #a letter sets the command; numbers after it repeat the same command, as svg allows
		switch ($command) {
			'M' { $x = Read-Number; $y = Read-Number; $path.StartFigure(); $command = 'L' } #a new figure begins at this point, and any numbers that follow are lines
			'L' { $nx = Read-Number; $ny = Read-Number; $path.AddLine([single]($x * $scale), [single]($y * $scale), [single]($nx * $scale), [single]($ny * $scale)); $x = $nx; $y = $ny }
			'H' { $nx = Read-Number; $path.AddLine([single]($x * $scale), [single]($y * $scale), [single]($nx * $scale), [single]($y * $scale)); $x = $nx }
			'V' { $ny = Read-Number; $path.AddLine([single]($x * $scale), [single]($y * $scale), [single]($x * $scale), [single]($ny * $scale)); $y = $ny }
			'A' {
				$r = Read-Number; $null = Read-Number; $null = Read-Number; $large = Read-Number; $sweep = Read-Number; $nx = Read-Number; $ny = Read-Number #radius twice, since the glyph's arcs are circular, then the rotation, which a circle has no use for, the two flags, and the end point
				#where the circle's center is, from the two end points, the radius, and the flags: svg's own arithmetic for an unrotated circular arc. Half the chord's vector, the distance from the chord's middle to the center along the chord's perpendicular, and which of the two mirror-image centers the flags choose
				$hx = ($x - $nx) / 2; $hy = ($y - $ny) / 2
				$h = [math]::Sqrt([math]::Max(0, ($r * $r - $hx * $hx - $hy * $hy) / ($hx * $hx + $hy * $hy))) #zero when the chord is the whole diameter, where both centers coincide at the chord's middle
				if ($large -eq $sweep) { $h = -$h }
				$cx = $h * $hy + ($x + $nx) / 2; $cy = -$h * $hx + ($y + $ny) / 2
				#gdi+ draws an arc from a start angle through a sweep, both in degrees, clockwise on screen since y points down, which is the same sense atan2 measures in these coordinates. The sweep flag says which way around, so the difference of the two angles is pushed to the right side of zero
				$start = [math]::Atan2($y - $cy, $x - $cx) * 180 / [math]::PI
				$end = [math]::Atan2($ny - $cy, $nx - $cx) * 180 / [math]::PI
				$delta = $end - $start
				if ($sweep -eq 1 -and $delta -lt 0) { $delta += 360 } elseif ($sweep -eq 0 -and $delta -gt 0) { $delta -= 360 }
				$path.AddArc([single](($cx - $r) * $scale), [single](($cy - $r) * $scale), [single](2 * $r * $scale), [single](2 * $r * $scale), [single]$start, [single]$delta) #the arc in the box of its full circle, its top left and its size, which is how gdi+ names a circle
				$x = $nx; $y = $ny
			}
			'Z' { $path.CloseFigure(); $command = '' }
			default { throw "unknown path command $command in $Glyph" }
		}
	}
	$path
}

foreach ($size in $Sizes) {
	$scale = $size / 16 #one grid unit in pixels at this size
	foreach ($name in $Colors.Keys) {
		$bitmap = New-Object System.Drawing.Bitmap -ArgumentList $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb) #transparent to start: the outside of the pill, and the hole through it, stay clear
		$graphics = [System.Drawing.Graphics]::FromImage($bitmap) #the drawing surface over the bitmap
		$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias #curved edges blend into whatever is beneath them, the taskbar
		$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality #treat pixel coordinates as the corners of pixels, so a shape from 4 to 12 covers exactly pixels 4 through 11, edge to edge; the default treats them as centers and would smear every edge across two pixels
		$path = Build-Path $scale
		$brush = New-Object System.Drawing.SolidBrush -ArgumentList ([System.Drawing.ColorTranslator]::FromHtml($Colors[$name])) #a fill color from #rrggbb or a named color like white
		$graphics.FillPath($brush, $path) #the pill, with the hole left by the fill rule
		$brush.Dispose(); $path.Dispose(); $graphics.Dispose()
		$bitmap.Save("$Stem-$name-$size.png", [System.Drawing.Imaging.ImageFormat]::Png)
		$bitmap.Dispose()
		"wrote    $(Split-Path $Stem -Leaf)-$name-$size.png"
	}
}
