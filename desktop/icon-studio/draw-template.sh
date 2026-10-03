#!/bin/zsh
# Draw the Mac's menu bar icon: glyph.svg in, tray-template.png out, here as the record and copied into src-tauri/icons/, where lifecycle.rs builds it into the program. The studio's other scripts are PowerShell and run on Windows; this one runs on the Mac, with ImageMagick from Homebrew, and the README's section on the menu bar icon says why the file is the size it is.
set -e
cd "$(dirname "$0")" #so it runs from anywhere, and the paths below are the studio's
sed 's/width="1024" height="1024" viewBox="0 0 16 16" fill="none"/width="36" height="36" viewBox="-1 -1 18 18"/' glyph.svg | magick -background none svg:- -define png:color-type=6 -strip tray-template.png #the drawing on an 18 unit canvas, one unit in from each edge, at 36 pixels, which is 18 points at Retina. The sed also drops the root element's fill="none", which ImageMagick's own SVG renderer would otherwise apply over the path's black and render nothing; and the define writes red, green, blue, and alpha channels, which include_image! insists on, where ImageMagick would otherwise save a black drawing as gray and alpha; and strip drops the dated text chunks ImageMagick would otherwise write, so a rerun on an unchanged drawing writes an identical file
inked=$(magick tray-template.png -format "%[fx:mean.a]" info:-) #the fraction of the square the glyph covers, read back from the file
if [[ "$inked" == "0" ]]; then echo "tray-template.png has nothing in it: the renderer dropped the fill"; exit 1; fi #the trap above, caught if the sed ever stops matching the drawing's root element
cp tray-template.png ../src-tauri/icons/tray-template.png
magick tray-template.png -format "tray-template.png: %w by %h, %[channels], and the glyph covers $(printf '%.0f' $((inked * 100))) percent of it\n" info:-
