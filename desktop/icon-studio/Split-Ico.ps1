<#
.\desktop\icon-studio\Split-Ico.ps1

Take an .ico apart into one PNG per layer, named Stem-<size>.png. It ran once on sheet.ico, the blank page as it arrived, to lay its eight sizes out as files a person can draw on, and it runs again only when a new sheet arrives.

An .ico is a small directory followed by the images it points at: six bytes of header, then sixteen bytes per layer giving its size and where its bytes start, then the layers themselves. A layer is stored one of two ways. Modern icons store each layer as a complete PNG file, which is copied out here as it is. Older tools store a bare Windows bitmap instead: a forty byte header, then the pixels as blue, green, red, alpha, one byte each, rows written from the bottom of the image up, then a one bit mask, one bit per pixel, that older Windows used for transparency before alpha existed. A bitmap layer is turned into a PNG by reading its pixels into a System.Drawing bitmap and saving that. Some drawing tools leave every alpha byte at zero and mean the transparency by the mask alone, and that case is honored when it comes up, which it did for this sheet.

	.\Split-Ico.ps1 -Ico sheet.ico -Stem sheet
#>

param(
	[Parameter(Mandatory)][string]$Ico, #the icon to take apart
	[Parameter(Mandatory)][string]$Stem #the path and name the layers are written under, as Stem-256.png, Stem-16.png, and so on
)
Add-Type -AssemblyName System.Drawing #gdi+, the drawing library every windows has, reached through .net
if (-not [IO.Path]::IsPathRooted($Stem)) { $Stem = Join-Path $PWD $Stem } #a full path, because a .net call given a relative one resolves it against the process's working directory, which is not the folder powershell is sitting in

$bytes = [IO.File]::ReadAllBytes((Resolve-Path $Ico))
$count = [BitConverter]::ToUInt16($bytes, 4) #how many layers, a two byte number at offset four; BitConverter reads little endian numbers out of a byte array, which is how every number in an .ico is stored
for ($i = 0; $i -lt $count; $i++) {
	$entry = 6 + $i * 16 #this layer's sixteen bytes in the directory
	$width = $bytes[$entry]; if ($width -eq 0) { $width = 256 } #one byte each for width and height, so 256, the largest size, is written as 0
	$height = $bytes[$entry + 1]; if ($height -eq 0) { $height = 256 }
	$size = [BitConverter]::ToUInt32($bytes, $entry + 8) #how many bytes the layer is
	$offset = [BitConverter]::ToUInt32($bytes, $entry + 12) #and where in the file it starts
	$out = "$Stem-$width.png"

	if ($bytes[$offset] -eq 0x89 -and $bytes[$offset + 1] -eq 0x50) { #a png starts with the bytes 0x89 and P, so this layer is already a png file
		[IO.File]::WriteAllBytes($out, $bytes[$offset..($offset + $size - 1)]) #copied out whole; the range operator slices the array
		"wrote    $(Split-Path $out -Leaf)  ${width}x${height}  png copied"
		continue
	}

	#a bitmap layer: a header, then pixels, then the mask
	$headerSize = [BitConverter]::ToUInt32($bytes, $offset) #the header says its own length, so the pixels start right after it
	$depth = [BitConverter]::ToUInt16($bytes, $offset + 14) #bits per pixel; only 32, four bytes a pixel with alpha, is handled here, because that's all a modern icon holds
	if ($depth -ne 32) { throw "only 32 bit bitmap layers are handled, and the ${width}x${height} layer is $depth bit" }
	$pixels = $offset + $headerSize
	$bitmap = New-Object System.Drawing.Bitmap -ArgumentList $width, $height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb) #an empty image with an alpha channel, filled in below one pixel at a time
	$anyAlpha = $false
	for ($y = 0; $y -lt $height; $y++) {
		$row = $pixels + ($height - 1 - $y) * $width * 4 #rows are stored bottom up, so the bitmap's last row is the image's first
		for ($x = 0; $x -lt $width; $x++) {
			$pixel = $row + $x * 4
			if ($bytes[$pixel + 3] -ne 0) { $anyAlpha = $true }
			$bitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($bytes[$pixel + 3], $bytes[$pixel + 2], $bytes[$pixel + 1], $bytes[$pixel])) #FromArgb takes alpha, red, green, blue, and the bytes sit blue, green, red, alpha, so they're read backwards
		}
	}
	if (-not $anyAlpha) { #every alpha is zero, so the layer means its transparency by the mask instead: one bit a pixel, each row padded to a multiple of four bytes, bottom up, a set bit meaning transparent
		$mask = $pixels + $width * $height * 4 #the mask starts right after the pixels
		$stride = [math]::Ceiling($width / 32) * 4 #bytes per mask row, rounded up to a four byte boundary
		for ($y = 0; $y -lt $height; $y++) {
			for ($x = 0; $x -lt $width; $x++) {
				$bit = ($bytes[$mask + ($height - 1 - $y) * $stride + ($x -shr 3)] -shr (7 - ($x -band 7))) -band 1 #the byte holding this pixel's bit, shifted so that bit is lowest, and masked to it; -shr shifts right, -band is bitwise and
				$color = $bitmap.GetPixel($x, $y)
				$bitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($(if ($bit) { 0 } else { 255 }), $color.R, $color.G, $color.B))
			}
		}
	}
	$bitmap.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
	$bitmap.Dispose()
	"wrote    $(Split-Path $out -Leaf)  ${width}x${height}  bitmap converted"
}
