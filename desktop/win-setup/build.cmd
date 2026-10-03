@echo off
rem Builds setup.exe, the stub win-setup.js appends a cabinet to, into the build folder. win-setup.js runs this for every release after writing build\stamp.h, which names the product, version, publisher, and icon for the resource script; run by hand, with no stamp written, the stub builds with the defaults in setup.rc.
setlocal
cd /d "%~dp0"

rem Find the newest Visual Studio with the C++ tools and load its developer environment, which puts cl, rc, and link on the path. vswhere is installed with every Visual Studio since 2017 at this fixed location.
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSPATH=%%i
if not defined VSPATH echo Visual Studio with the C++ tools was not found & exit /b 1
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -no_logo || exit /b 1

rem The stamp header setup.rc includes, empty when nothing wrote one, so the defaults apply
if not exist build mkdir build
if not exist build\stamp.h type nul > build\stamp.h

rem Plain C at warning level 4, UTF-16 throughout, the static runtime so it needs no redistributable, and control flow guard. The linker embeds no manifest of its own, since setup.rc carries one, and cabinet.dll and bcrypt.dll are delay-loaded so they are looked up only after the program has restricted library loading to the system folder.
rc /nologo /fo build\setup.res setup.rc || exit /b 1
cl /nologo /W4 /O1 /MT /guard:cf /D UNICODE /D _UNICODE /c /Fobuild\ setup.c || exit /b 1
link /nologo /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /GUARD:CF /MANIFEST:NO /DELAYLOAD:cabinet.dll /DELAYLOAD:bcrypt.dll /OUT:build\setup.exe build\setup.obj build\setup.res kernel32.lib user32.lib shell32.lib ole32.lib advapi32.lib uuid.lib cabinet.lib bcrypt.lib delayimp.lib || exit /b 1

echo built build\setup.exe
