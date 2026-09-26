<#
.\desktop\icon-studio\Test-Ico.ps1

Ask the Windows shell for every layer of an .ico, the way Explorer does, and report what came back. This is the check to run on a packed icon, and the one .NET can't do: System.Drawing.Icon refuses PNG layers it didn't write itself, while SHDefExtractIcon reads them all. A layer that comes back at its own size with the right number of solid pixels is a layer Explorer will draw.

	.\Test-Ico.ps1 -Ico torrent.ico
#>

param(
	[Parameter(Mandatory)][string]$Ico,
	[int[]]$Sizes = @(256, 64, 48, 40, 32, 24, 20, 16)
)
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class IconStudioShell {
	[DllImport("shell32.dll", CharSet=CharSet.Unicode)] public static extern int SHDefExtractIconW(string path, int index, uint flags, out IntPtr large, out IntPtr small, uint size);
	[DllImport("user32.dll")] public static extern bool DestroyIcon(IntPtr h);
}
'@
#the two windows functions this needs, declared in a few lines of C# that powershell compiles on the spot: SHDefExtractIcon hands back the icon at a requested size as a handle, the shell's own way of reading an icon file, and DestroyIcon gives the handle back when we're done with it. Add-Type compiles once per powershell session, so running this twice in one window is fine

$file = (Resolve-Path $Ico).Path
"$(Split-Path $file -Leaf): $((Get-Item $file).Length) bytes"
foreach ($size in $Sizes) {
	$large = [IntPtr]::Zero; $small = [IntPtr]::Zero #the two handles the call fills in; [ref] below is how powershell passes a variable for a function to write into
	$result = [IconStudioShell]::SHDefExtractIconW($file, 0, 0, [ref]$large, [ref]$small, [uint32]$size) #index 0 is the first icon in the file, the only one an .ico has; flags 0 means no special treatment
	if ($result -ne 0 -or $large -eq [IntPtr]::Zero) { "  {0,3}: the shell could not extract this size, result {1}" -f $size, $result; continue }
	$bitmap = [System.Drawing.Icon]::FromHandle($large).ToBitmap() #the handle as a .net image, so its pixels can be counted
	$opaque = 0
	for ($y = 0; $y -lt $bitmap.Height; $y++) { for ($x = 0; $x -lt $bitmap.Width; $x++) { if ($bitmap.GetPixel($x, $y).A -gt 200) { $opaque++ } } }
	"  {0,3}: {1}x{2}, {3} solid pixels" -f $size, $bitmap.Width, $bitmap.Height, $opaque
	$bitmap.Dispose()
	[IconStudioShell]::DestroyIcon($large) | Out-Null #give the handle back; Out-Null drops the true it returns, which would otherwise print
}
