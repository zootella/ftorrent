=How to Build libtorrent in Visual Studio=

''<font face=serif>for Fun and Profit</font>''

http://www.rasterbar.com/products/libtorrent/building.html do section building with BBv2<br>
http://www.rasterbar.com/products/libtorrent/vs2005_build_notes.html out of date<br>
http://code.rasterbar.com/libtorrent/wiki/Building down<br>
http://github.com/zootella/ltorrent code and project<br>

==Windows==

Windows XP SP3 and [http://windowsupdate.microsoft.com Windows Update]<br>
[[Visual Studio]] 2008 and SP1<br>
[http://www.microsoft.com/downloads/details.aspx?FamilyID=c17ba869-9671-4330-a63e-1fd44e0e2505&displaylang=en Windows SDK] Version 7, published July 24 2009<br>

<font color="gray">'''----snapshot vs2008----'''</font>

[[Cygwin]] including ''perl'' and ''nasm''

==Code==

===Place Code===

[http://www.openssl.org/ openssl.org]
download a file like ''openssl-0.9.8n.tar.gz''<br>
Unzip to path like:<br>
''C:\openssl\Makefile.org''

[http://www.boost.org/ boost.org]
download a file like ''boost_1_43_0.zip''<br>
Unzip to path like:<br>
''C:\boost\boost-build.jam''

https://libtorrent.svn.sourceforge.net/svnroot/libtorrent/tags
find the highest numbered tag
 svn co <nowiki>https://libtorrent.svn.sourceforge.net/svnroot/libtorrent/tags/libtorrent-0_15_0</nowiki> libtorrent
Move to path like<br>
''C:\libtorrent\project-root.jam''

===Make bjam===

Visual Studio 2008 Command Prompt
 cd C:\boost\tools\jam\src
 build.bat

Creates the file:<br>
''C:\boost\tools\jam\src\bin.ntx86\bjam.exe''<br>
Copy ''bjam.exe'' to the ''build'' folder, alongside the ''v2'' folder:<br>
''C:\boost\tools\build\bjam.exe''

Open in Notepad:<br>
''C:\boost\tools\build\v2\user-config.jam''<br>
Uncomment the line ''using msvc ;''

===Build libraries===

Visual Studio 2008 Command Prompt

 set PATH=%PATH%;C:\boost\tools\build;C:\cygwin\bin

Build OpenSSL
 cd C:\openssl
 perl Configure --openssldir=C:/openssl VC-WIN32
 ms\do_nasm.bat
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
 bjam --without-python --toolset=msvc boost=source link=static runtime-link=static character-set=unicode variant=debug openssl=off
 bjam --without-python --toolset=msvc boost=source link=static runtime-link=static character-set=unicode variant=release

===Move libtorrent.lib===

Move from:<br>
''C:\libtorrent\bin\msvc-9.0\debug\boost-source\link-static\threading-multi\libtorrent.lib''<br>
''C:\libtorrent\bin\msvc-9.0\release\boost-source\link-static\threading-multi\libtorrent.lib''<br>
Rename to:<br>
''C:\libtorrent\lib\libtorrent-debug.lib''<br>
''C:\libtorrent\lib\libtorrent-release.lib''<br>

==Visual Studio==

===Include and Lib===

Tools, Options, Projects and Solutions, VC++ Directories

Include files
*C:\libtorrent\include
*C:\libtorrent\zlib
*C:\boost
*C:\openssl\include

Library files
*C:\libtorrent\lib
*C:\boost\stage\lib
*C:\openssl\lib

===Code===

 git clone git@github.com:zootella/ltorrent.git

===Properties===

Project, Properties, Configuration Properties
*General, Use of ATL: Not Using ATL
*General, Character Set: Use Unicode Character Set
*C/C++, Code Generation, Enable Minimal Rebuild: No
*C/C++, Code Generation, Enable C++ Exceptions: Yes With SEH Exceptions (/EHa)
*C/C++, Code Generation, Basic Runtime Checks: Default
*C/C++, Code Generation, Runtime Library: change /MDd to /MTd and /MD to /MT to get rid of dll
*C/C++, Precompiled Headers, Create/Use Precompiled Header: Not Using Precompiled Headers
*Linker, Manifest File, Generate Manifest: No
*Manifest Tool, Input and Output, Embed Manifest: No
Check all with Configuration Debug and Release<br>

Project, Properties, Configuration Properties, Release
*General, Whole Program Optimization: Use Link Time Code Generation
*C/C++, Optimization, Optimization: Minimize Size (/O1)

Project, Properties, Configuration Properties, C/C++, Preprocessor, Preprocessor Definitions, Debug/Release
*WIN32
*_DEBUG/NDEBUG
*_WINDOWS
*_WIN32_WINNT=0x0501
*BOOST_ALL_NO_LIB
*BOOST_THREAD_USE_LIB
*_FILE_OFFSET_BITS=64
*WITH_SHIPPED_GEOIP_H

Project, Properties, Configuration Properties, Linker, Input, Additional Dependencies, Debug
*comctl32.lib
*libboost_date_time-vc90-mt-sgd.lib
*libboost_filesystem-vc90-mt-sgd.lib
*libboost_system-vc90-mt-sgd.lib
*libboost_thread-vc90-mt-sgd.lib
*libtorrent-debug.lib

Release
*comctl32.lib
*ssleay32.lib
*libeay32.lib
*libboost_date_time-vc90-mt-s.lib
*libboost_filesystem-vc90-mt-s.lib
*libboost_system-vc90-mt-s.lib
*libboost_thread-vc90-mt-s.lib
*libtorrent-release.lib

See the whole command line at ''C/C++, Command Line'' and ''Linker, Command Line''

==Notes==

===New Project===

Visual Studio 2008<br>
File, New, Project<br>
Project types: Visual C++, Win32, Win32 Project<br>
Name: ltorrent<br>
Location: C:\Documents<br>
Uncheck Create directory for solution<br>
OK<br>
Application Settings, check Empty Project<br>
Finish<br>

Project, Add Existing Item<br>
select all the ''.h'' files, Add<br>
Do it again for the ''.cpp'' files<br>

===Resources===

Optional steps to start the resources from scratch
*menu File, Add New Item
*choose Resource File (.rc), enter name, Open

===Add Code===

Drop libtorrentwrapper.cpp next to Hello.cpp<br>
''C:\Documents\Visual Studio 2008\Projects\Hello\libtorrentwrapper.cpp''<br>
Project, Add Existing Item, libtorrentwrapper.cpp, Add<br>

Comment out ''class blacklist_ip_filter_callback'' and fix errors<br>
