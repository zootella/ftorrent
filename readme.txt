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

[[Cygwin]] including ''perl''

==Code==

===Place Code===

[http://www.openssl.org/ openssl.org]
download a file like ''openssl-0.9.8l.tar.gz''<br>
Unzip to path like:<br>
''C:\openssl\Makefile.bak''

[http://www.boost.org/ boost.org]
download a file like ''boost_1_42_0.zip''<br>
Unzip to path like:<br>
''C:\boost_1_42_0\boost-build.jam''

https://libtorrent.svn.sourceforge.net/svnroot/libtorrent/tags
find the highest numbered tag
 svn co <nowiki>https://libtorrent.svn.sourceforge.net/svnroot/libtorrent/tags/libtorrent-0_14_8</nowiki> libtorrent
Move to path like<br>
''C:\libtorrent\project-root.jam''

===Make bjam===

Visual Studio 2008 Command Prompt
 cd C:\boost_1_42_0\tools\jam\src
 build.bat

Creates the file:<br>
''C:\boost_1_42_0\tools\jam\src\bin.ntx86\bjam.exe''<br>
Copy ''bjam.exe'' to the ''build'' folder, alongside the ''v2'' folder:<br>
''C:\boost_1_42_0\tools\build\bjam.exe''

Open in Notepad:<br>
''C:\boost_1_42_0\tools\build\v2\user-config.jam''<br>
Uncomment the line ''using msvc ;''

===Build libraries===

Visual Studio 2008 Command Prompt

 set PATH=%PATH%;C:\boost_1_42_0\tools\build;C:\cygwin\bin

Build OpenSSL
 cd C:\openssl
 perl Configure --openssldir=C:/openssl VC-WIN32
 ms\do_masm
 nmake -f ms\nt.mak > result1.txt
 nmake -f ms\nt.mak test > result2.txt
 nmake -f ms\nt.mak install > result3.txt

Build boost
 cd C:\boost_1_42_0
 bjam --toolset=msvc link=static runtime-link=static variant=debug > result4.txt
 bjam --toolset=msvc link=static runtime-link=static variant=release > result5.txt

Build libtorrent
 cd C:\libtorrent
 set BOOST_ROOT=C:\boost_1_42_0
 set INCLUDE=%INCLUDE%;C:\openssl\include
 set LIB=%LIB%;C:\openssl\out32
 bjam --toolset=msvc boost=source link=static runtime-link=static variant=debug character-set=unicode --without-python > result6.txt
 bjam --toolset=msvc boost=source link=static runtime-link=static variant=release character-set=unicode --without-python > result7.txt

===Move libtorrent.lib===

Move from:<br>
''C:\libtorrent\bin\msvc-9.0\debug\boost-source\link-static\threading-multi\libtorrent.lib''<br>
''C:\libtorrent\bin\msvc-9.0\release\boost-source\link-static\threading-multi\libtorrent.lib''<br>
Rename to:<br>
''C:\libtorrent\lib\libtorrent-debug.lib''<br>
''C:\libtorrent\lib\libtorrent-release.lib''<br>

==Visual Studio==

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

===Include and Lib===

Tools, Options, Projects and Solutions, VC++ Directories

Include files
*C:\openssl\include
*C:\boost_1_42_0
*C:\libtorrent\include
*C:\libtorrent\zlib

Library files
*C:\openssl\lib
*C:\boost_1_42_0\stage\lib
*C:\libtorrent\lib

===Properties===

Project, Properties, Configuration Properties
*General, Use of ATL: Not Using ATL
*General, Character Set: Use Unicode Character Set
*C/C++, Preprocessor, Preprocessor Definitions: Add _WIN32_WINNT=0x0501 to the bottom of the list
*C/C++, Code Generation, Runtime Library: change /MDd to /MTd and /MD to /MT to get rid of dll
*C/C++, Precompiled Headers, Create/Use Precompiled Header: Not Using Precompiled Headers
*Linker, Manifest File, Generate Manifest: No
*Manifest Tool, Input and Output, Embed Manifest: No
At the end, check all with Configuration Debug and Release

Check out the whole command line at ''C/C++, Command Line'' and ''Linker, Command Line''

Linker, Input, Additional Dependencies, Debug
*advapi32.lib
*comctl32.lib
*ssleay32.lib
*libeay32.lib
*libboost_date_time-vc90-mt-sgd.lib
*libboost_filesystem-vc90-mt-sgd.lib
*libboost_system-vc90-mt-sgd.lib
*libboost_thread-vc90-mt-sgd.lib
*libtorrent-debug.lib

Release
*advapi32.lib
*comctl32.lib
*ssleay32.lib
*libeay32.lib
*libboost_date_time-vc90-mt-s.lib
*libboost_filesystem-vc90-mt-s.lib
*libboost_system-vc90-mt-s.lib
*libboost_thread-vc90-mt-s.lib
*libtorrent-release.lib

Maybe try later
*Configuration Release, General, Whole Program Optimization: Yes
*Configuration Release, C/C++, Optimization, Optimization: Minimize Size 01

Optional steps to start the resources from scratch
*menu File, Add New Item
*choose Resource File (.rc), enter name, Open

==Code==

===Add Code===

Drop libtorrentwrapper.cpp next to Hello.cpp<br>
''C:\Documents\Visual Studio 2008\Projects\Hello\libtorrentwrapper.cpp''<br>
Project, Add Existing Item, libtorrentwrapper.cpp, Add<br>

Comment out ''class blacklist_ip_filter_callback'' and fix errors<br>