<#
.\desktop\icon-studio\Join-Ico.ps1

Gather Stem-<size>.png files into one .ico. Every layer is stored as a PNG, which Windows has read since Vista and which is how tauri icon writes the application icon too, and the largest layer comes first, which is the order Windows expects. The format is the one Split-Ico.ps1 reads: a six byte header, sixteen bytes of directory per layer, then the layers, so this is that script's reverse.

Check the result with Test-Ico.ps1, which asks the shell, and not by loading it in .NET: System.Drawing.Icon refuses PNG layers it didn't write itself, while SHDefExtractIcon, which is what Explorer uses, reads every one.

	.\Join-Ico.ps1 -Stem torrent -Ico torrent.ico
#>

param(
	[Parameter(Mandatory)][string]$Stem, #the path and name the layers are read from, as Stem-256.png, Stem-16.png, and so on
	[Parameter(Mandatory)][string]$Ico #the icon to write
)
if (-not [IO.Path]::IsPathRooted($Ico)) { $Ico = Join-Path $PWD $Ico } #a full path, because a .net call given a relative one resolves it against the process's working directory, which is not the folder powershell is sitting in

$folder = Split-Path $Stem -Parent; if (-not $folder) { $folder = '.' }
$prefix = (Split-Path $Stem -Leaf) + '-'
$layers = @() #one record per layer, holding its bytes and its size
foreach ($file in Get-ChildItem $folder -Filter "$prefix*.png") {
	$png = [IO.File]::ReadAllBytes($file.FullName)
	$width = [BitConverter]::ToUInt32([byte[]]$png[19..16], 0) #a png's width and height sit at bytes 16 and 20 of its header, big endian, so the four bytes are read in reverse to make the little endian number BitConverter expects
	$height = [BitConverter]::ToUInt32([byte[]]$png[23..20], 0)
	$layers += [pscustomobject]@{Png = $png; Width = $width; Height = $height} #the bytes ride inside an object on purpose: a bare byte array sent down a powershell pipeline is unrolled into single bytes
}
if ($layers.Count -eq 0) { throw "no $prefix*.png in $folder" }
$layers = @($layers | Sort-Object Width -Descending) #largest first

$stream = New-Object IO.MemoryStream #the file is assembled in memory and written once at the end
$writer = New-Object IO.BinaryWriter($stream) #writes little endian numbers of the width its argument's type says: [uint16] two bytes, [uint32] four
$writer.Write([uint16]0) #reserved, always zero
$writer.Write([uint16]1) #type 1 is an icon; 2 would be a cursor
$writer.Write([uint16]$layers.Count)
$offset = 6 + $layers.Count * 16 #the first layer's bytes sit right after the directory
foreach ($layer in $layers) {
	$writer.Write([byte]$(if ($layer.Width -eq 256) { 0 } else { $layer.Width })) #one byte each for width and height, so 256 is written as 0
	$writer.Write([byte]$(if ($layer.Height -eq 256) { 0 } else { $layer.Height }))
	$writer.Write([byte]0) #colors in a palette; none, these are true color
	$writer.Write([byte]0) #reserved
	$writer.Write([uint16]1) #color planes, one
	$writer.Write([uint16]32) #bits per pixel
	$writer.Write([uint32]$layer.Png.Length)
	$writer.Write([uint32]$offset)
	$offset += $layer.Png.Length #the next layer starts where this one ends
}
foreach ($layer in $layers) { $writer.Write([byte[]]$layer.Png) } #the layers themselves, in the same order the directory named them
$writer.Flush()
[IO.File]::WriteAllBytes($Ico, $stream.ToArray())
"wrote    $(Split-Path $Ico -Leaf)  $($layers.Count) layers  $(($layers | ForEach-Object { $_.Width }) -join ' ')"
