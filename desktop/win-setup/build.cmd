@echo off
rem Builds setup.exe, the stub win-setup.js appends a cabinet to, into the build folder. win-setup.js runs this for every release after writing build\stamp.h, which names the product and everything else the stub knows, for setup.rc and setup.c both; run by hand with no stamp, this writes one with placeholder names, so the stub builds and says what it is.
setlocal
cd /d "%~dp0"

rem Find the newest Visual Studio with the C++ tools and load its developer environment, which puts cl, rc, and link on the path. vswhere is installed with every Visual Studio since 2017 at this fixed location, which goes on the path too, because VsDevCmd.bat itself calls vswhere by bare name and complains, harmlessly, when it is not there.
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
for /f "usebackq delims=" %%i in (`vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSPATH=%%i
if not defined VSPATH echo Visual Studio with the C++ tools was not found & exit /b 1
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -no_logo || exit /b 1

rem The stamp header, when nothing wrote one: the same defines win-setup.js writes, with placeholder values and an empty uninstall list. Each line puts the redirection first, because cmd reads a line ending in 0>> as a redirection of handle zero and drops the zero
if not exist build mkdir build
if not exist build\stamp.h (
	>build\stamp.h echo // written by build.cmd for a build by hand; win-setup.js writes the real one
	>>build\stamp.h echo #define SETUP_ICON "..\\src-tauri\\icons\\icon.ico"
	>>build\stamp.h echo #define SETUP_BRAND_NAME "Example"
	>>build\stamp.h echo #define SETUP_BRAND_STEM "example"
	>>build\stamp.h echo #define SETUP_IDENTIFIER "com.example.example"
	>>build\stamp.h echo #define SETUP_PUBLISHER ""
	>>build\stamp.h echo #define SETUP_COPYRIGHT ""
	>>build\stamp.h echo #define SETUP_HOMEPAGE ""
	>>build\stamp.h echo #define SETUP_VERSION 0,0,0,0
	>>build\stamp.h echo #define SETUP_VERSION_TEXT "0.0.0"
	>>build\stamp.h echo #define SETUP_SIZE_KB 0
	>>build\stamp.h echo #define SETUP_UNINSTALL
)

rem Plain C at warning level 4, UTF-16 throughout, source read as UTF-8 so the stamp's text can be anything, the static runtime so it needs no redistributable, and control flow guard. The linker embeds no manifest of its own, since setup.rc carries one, and cabinet.dll is delay-loaded so it is looked up only after the program has restricted library loading to the system folder.
rc /nologo /fo build\setup.res setup.rc || exit /b 1
cl /nologo /W4 /O1 /MT /utf-8 /guard:cf /D UNICODE /D _UNICODE /c /Fobuild\ setup.c || exit /b 1
link /nologo /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /GUARD:CF /MANIFEST:NO /DELAYLOAD:cabinet.dll /OUT:build\setup.exe build\setup.obj build\setup.res kernel32.lib user32.lib shell32.lib ole32.lib advapi32.lib uuid.lib cabinet.lib delayimp.lib || exit /b 1

echo built build\setup.exe
