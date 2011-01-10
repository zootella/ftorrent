
 _   _                 _          ____        _ _     _ 
| | | | _____      __ | |_ ___   | __ ) _   _(_) | __| |
| |_| |/ _ \ \ /\ / / | __/ _ \  |  _ \| | | | | |/ _` |
|  _  | (_) \ V  V /  | || (_) | | |_) | |_| | | | (_| |
|_| |_|\___/ \_/\_/    \__\___/  |____/ \__,_|_|_|\__,_|
 _ _ _     _                            _     _       
| (_) |__ | |_ ___  _ __ _ __ ___ _ __ | |_  (_)_ __  
| | | '_ \| __/ _ \| '__| '__/ _ \ '_ \| __| | | '_ \ 
| | | |_) | || (_) | |  | | |  __/ | | | |_  | | | | |
|_|_|_.__/ \__\___/|_|  |_|  \___|_| |_|\__| |_|_| |_|
__     ___                 _   ____  _             _ _       
\ \   / (_)___ _   _  __ _| | / ___|| |_ _   _  __| (_) ___  
 \ \ / /| / __| | | |/ _` | | \___ \| __| | | |/ _` | |/ _ \ 
  \ V / | \__ \ |_| | (_| | |  ___) | |_| |_| | (_| | | (_) |
   \_/  |_|___/\__,_|\__,_|_| |____/ \__|\__,_|\__,_|_|\___/ 
                                                             
for Fun and Profit

==Links==

http://www.rasterbar.com/products/libtorrent/building.html
Do section building with BBv2

http://www.rasterbar.com/products/libtorrent/vs2005_build_notes.html
Out of date

http://code.rasterbar.com/libtorrent/wiki/Building
Down

==Windows==

http://windowsupdate.microsoft.com
Windows XP SP3 and Windows Update
Visual Studio 2008 and SP1

http://www.microsoft.com/downloads/details.aspx?FamilyID=c17ba869-9671-4330-a63e-1fd44e0e2505&displaylang=en 
Windows SDK Version 7, published July 24 2009

==Perl==

http://www.activestate.com/activeperl
Download a file like ActivePerl-5.12.2.1203-MSWin32-x86-294165.msi
Install with all the defaults

==Place==

http://www.openssl.org/
Download a file like openssl-1.0.0c.tar.gz
Unzip to path like:
C:\openssl\Makefile.org

http://www.boost.org/
Download a file like boost_1_45_0.zip
Unzip to path like:
C:\boost\boost-build.jam

https://libtorrent.svn.sourceforge.net/svnroot/libtorrent/tags
Find the highest numbered tag
svn co https://libtorrent.svn.sourceforge.net/svnroot/libtorrent/tags/libtorrent-0_15_4 libtorrent
Move to path like:
C:\libtorrent\project-root.jam

==Make bjam==

Visual Studio 2008 Command Prompt
cd C:\boost\tools\build\v2\engine\src
build.bat

Creates the file:
C:\boost\tools\build\v2\engine\src\bin.ntx86\bjam.exe
Copy bjam.exe to the build folder, alongside the v2 folder:
C:\boost\tools\build\bjam.exe

Open in Notepad:
C:\boost\tools\build\v2\user-config.jam
Uncomment the line using msvc ;

==Build libraries==

Visual Studio 2008 Command Prompt
set PATH=%PATH%;C:\boost\tools\build

Build OpenSSL
cd C:\openssl
perl Configure --openssldir=C:/openssl VC-WIN32
ms\do_ms.bat
nmake -f ms\nt.mak
nmake -f ms\nt.mak test
nmake -f ms\nt.mak install

Build boost
cd C:\boost
bjam --toolset=msvc link=static runtime-link=static variant=debug
bjam --toolset=msvc link=static runtime-link=static variant=release

Build libtorrent
cd C:\libtorrent
set BOOST_ROOT=C:\boost
set INCLUDE=%INCLUDE%;C:\openssl\include
set LIB=%LIB%;C:\openssl\out32
bjam --without-python --toolset=msvc boost=source link=static runtime-link=static character-set=unicode variant=debug
bjam --without-python --toolset=msvc boost=source link=static runtime-link=static character-set=unicode variant=release

==Move libtorrent.lib==

Move from:
C:\libtorrent\bin\msvc-9.0\debug\boost-source\link-static\runtime-link-static\threading-multi\libtorrent.lib
C:\libtorrent\bin\msvc-9.0\release\boost-source\link-static\runtime-link-static\threading-multi\libtorrent.lib
Rename to:
C:\libtorrent\lib\libtorrent-debug.lib
C:\libtorrent\lib\libtorrent-release.lib

==Include and lib==

Tools, Options, Projects and Solutions, VC++ Directories, Include files
C:\libtorrent\include
C:\libtorrent\zlib
C:\boost
C:\openssl\include

Library files
C:\libtorrent\lib
C:\boost\stage\lib
C:\openssl\lib

==Code==

git clone git@github.com:zootella/ftorrent.git

==Or, make a new project==

File, New, Project
Project types: Visual C++, Win32, Win32 Project
Name: ftorrent
Location: C:\Documents
Uncheck Create directory for solution
OK
Application Settings, check Empty Project
Finish

Place files in the ftorrent folder and then drag them into Visual Studio
*.h
*.cpp
*.ico
ftorrent.rc
ftorrent.exe.manifest

==Or, start resources from scratch==

File, Add New Item
choose Resource File (.rc), enter name, Open

==Properties==

Project, Properties, Configuration Properties, Configuration: Debug | Release
General, Use of ATL: Not Using ATL
General, Character Set: Use Unicode Character Set
General, Whole Program Optimization: No Whole Program Optimization | Use Link Time Code Generation
C/C++, Optimization, Optimization: Disabled (/Od) | Minimize Size (/O1)
C/C++, Code Generation, Enable Minimal Rebuild: Yes (/Gm) | No
C/C++, Code Generation, Enable C++ Exceptions: Yes With SEH Exceptions (/EHa)
C/C++, Code Generation, Basic Runtime Checks: Both | Default
C/C++, Code Generation, Runtime Library: Multi-threaded Debug (/MTd) | Multi-threaded (/MT)
C/C++, Precompiled Headers, Create/Use Precompiled Header: Not Using Precompiled Headers
Linker, Manifest File, Generate Manifest: No
Manifest Tool, Input and Output, Embed Manifest: No

Project, Properties, Configuration Properties, C/C++, Preprocessor, Preprocessor Definitions, Debug/Release
WIN32
_DEBUG/NDEBUG
_WINDOWS
_WIN32_WINNT=0x0501
BOOST_ALL_NO_LIB
BOOST_THREAD_USE_LIB
_FILE_OFFSET_BITS=64
WITH_SHIPPED_GEOIP_H

Project, Properties, Configuration Properties, Linker, Input, Additional Dependencies, Debug
comctl32.lib
ssleay32.lib
libeay32.lib
libboost_date_time-vc90-mt-sgd.lib
libboost_filesystem-vc90-mt-sgd.lib
libboost_system-vc90-mt-sgd.lib
libboost_thread-vc90-mt-sgd.lib
libtorrent-debug.lib

Release
comctl32.lib
ssleay32.lib
libeay32.lib
libboost_date_time-vc90-mt-s.lib
libboost_filesystem-vc90-mt-s.lib
libboost_system-vc90-mt-s.lib
libboost_thread-vc90-mt-s.lib
libtorrent-release.lib

See the whole command line at C/C++, Command Line and Linker, Command Line

==The end==
