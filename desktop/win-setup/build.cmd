@echo off
rem Builds Create.exe and Setup.exe from the command line with the Visual Studio toolchain, the way the 2005 project files did in the IDE: the same defines, optimization for size, the static runtime, warning level 3, and a windows subsystem linked with unreferenced code removed. The project files are from Visual Studio .NET 2003, which no current Visual Studio can open, so this script is their flags written out. Outputs land in each program's Release folder, as they did then.
setlocal
cd /d "%~dp0"

rem Find the newest Visual Studio with the C++ tools and load its developer environment, which puts cl, rc, and link on the path. vswhere is installed with every Visual Studio since 2017 at this fixed location.
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSPATH=%%i
if not defined VSPATH echo Visual Studio with the C++ tools was not found & exit /b 1
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x86 -host_arch=x64 -no_logo || exit /b 1

rem The compiler flags the project files set: /O1 minimize size, /MT the static runtime (the 2003 single-threaded static library /ML no longer exists, and /MT is its successor), /W3, and the defines; _MBCS is the project files' character set 2, 8-bit strings, which is why the code calls the ANSI versions of every function
set CL_FLAGS=/nologo /W3 /O1 /MT /EHsc /D WIN32 /D NDEBUG /D _WINDOWS /D _MBCS
rem The linker flags: a windows program with no console, and /OPT:REF /OPT:ICF to drop unreferenced functions and fold identical ones. The project files also asked for a program database, which is left out here because it writes the build machine's paths into the executable
set LINK_FLAGS=/nologo /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF

rem zlib once, into its own Release folder; each program links every object and the linker keeps only the functions it reaches
if not exist ZLib\Release mkdir ZLib\Release
cl %CL_FLAGS% /c /FoZLib\Release\ ZLib\adler32.c ZLib\compress.c ZLib\crc32.c ZLib\deflate.c ZLib\infback.c ZLib\inffast.c ZLib\inflate.c ZLib\inftrees.c ZLib\trees.c ZLib\uncompr.c ZLib\zutil.c || exit /b 1

rem Setup.exe, the small extracting program
if not exist Setup\Release mkdir Setup\Release
rc /nologo /fo Setup\Release\Setup.res Setup\Setup.rc || exit /b 1
cl %CL_FLAGS% /c /FoSetup\Release\ Setup\Setup.cpp || exit /b 1
link %LINK_FLAGS% /OUT:Setup\Release\Setup.exe Setup\Release\Setup.obj Setup\Release\Setup.res ZLib\Release\*.obj kernel32.lib user32.lib shell32.lib ole32.lib || exit /b 1

rem Create.exe, the dialog box that makes setup programs
if not exist Create\Release mkdir Create\Release
rc /nologo /fo Create\Release\Create.res Create\Create.rc || exit /b 1
cl %CL_FLAGS% /c /FoCreate\Release\ Create\Create.cpp || exit /b 1
link %LINK_FLAGS% /OUT:Create\Release\Create.exe Create\Release\Create.obj Create\Release\Create.res ZLib\Release\*.obj kernel32.lib user32.lib gdi32.lib shell32.lib ole32.lib comdlg32.lib advapi32.lib || exit /b 1

echo built Setup\Release\Setup.exe and Create\Release\Create.exe
