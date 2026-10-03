@echo off
rem Builds setup.exe, the stub win-setup.js appends a cabinet to, into the build folder; and after it the 2005 Create.exe, from the command line with the flags its Visual Studio .NET 2003 project file set, which no current Visual Studio can open, into its Release folder as then. The 2005 Setup.cpp is no longer built: setup.c is its descendant.
setlocal
cd /d "%~dp0"

rem Find the newest Visual Studio with the C++ tools and load its developer environment, which puts cl, rc, and link on the path. vswhere is installed with every Visual Studio since 2017 at this fixed location.
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSPATH=%%i
if not defined VSPATH echo Visual Studio with the C++ tools was not found & exit /b 1
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -no_logo || exit /b 1

rem setup.exe, the stub. Plain C at warning level 4, UTF-16 throughout, the static runtime so it needs no redistributable, and control flow guard. The linker embeds no manifest of its own, since setup.rc carries one, and cabinet.dll and bcrypt.dll are delay-loaded so they are looked up only after the program has restricted library loading to the system folder.
if not exist build mkdir build
rc /nologo /fo build\setup.res setup.rc || exit /b 1
cl /nologo /W4 /O1 /MT /guard:cf /D UNICODE /D _UNICODE /c /Fobuild\ setup.c || exit /b 1
link /nologo /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /GUARD:CF /MANIFEST:NO /DELAYLOAD:cabinet.dll /DELAYLOAD:bcrypt.dll /OUT:build\setup.exe build\setup.obj build\setup.res kernel32.lib user32.lib shell32.lib ole32.lib cabinet.lib bcrypt.lib delayimp.lib || exit /b 1

rem The compiler flags the 2005 project file set: /O1 minimize size, /MT the static runtime (the 2003 single-threaded static library /ML no longer exists, and /MT is its successor), /W3, and the defines; _MBCS is the project file's character set 2, 8-bit strings, which is why the code calls the ANSI versions of every function
set CL_FLAGS=/nologo /W3 /O1 /MT /EHsc /D WIN32 /D NDEBUG /D _WINDOWS /D _MBCS
rem The linker flags: a windows program with no console, and /OPT:REF /OPT:ICF to drop unreferenced functions and fold identical ones. The project file also asked for a program database, which is left out here because it writes the build machine's paths into the executable
set LINK_FLAGS=/nologo /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF

rem zlib once, into its own Release folder, for the 2005 creator; the linker keeps only the functions it reaches
if not exist ZLib\Release mkdir ZLib\Release
cl %CL_FLAGS% /c /FoZLib\Release\ ZLib\adler32.c ZLib\compress.c ZLib\crc32.c ZLib\deflate.c ZLib\infback.c ZLib\inffast.c ZLib\inflate.c ZLib\inftrees.c ZLib\trees.c ZLib\uncompr.c ZLib\zutil.c || exit /b 1

rem Create.exe, the 2005 dialog box that makes setup programs, until win-setup.js takes over its job
if not exist Create\Release mkdir Create\Release
rc /nologo /fo Create\Release\Create.res Create\Create.rc || exit /b 1
cl %CL_FLAGS% /c /FoCreate\Release\ Create\Create.cpp || exit /b 1
link %LINK_FLAGS% /OUT:Create\Release\Create.exe Create\Release\Create.obj Create\Release\Create.res ZLib\Release\*.obj kernel32.lib user32.lib gdi32.lib shell32.lib ole32.lib comdlg32.lib advapi32.lib || exit /b 1

echo built build\setup.exe and Create\Release\Create.exe
