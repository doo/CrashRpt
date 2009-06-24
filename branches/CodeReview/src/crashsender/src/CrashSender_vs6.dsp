# Microsoft Developer Studio Project File - Name="CrashSender_vs6" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=CrashSender_vs6 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "CrashSender_vs6.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "CrashSender_vs6.mak" CFG="CrashSender_vs6 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "CrashSender_vs6 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "CrashSender_vs6 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "CrashSender_vs6 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "CrashSender_vs6 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\tinyxml" /I "..\..\zip_utils" /I "..\..\CrashRpt\src" /I "..\..\CrashRpt\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  wininet.lib comsupp.lib wsock32.lib Rpcrt4.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\..\bin\CrashSenderd.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "CrashSender_vs6 - Win32 Release"
# Name "CrashSender_vs6 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "tinyxml"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\tinyxml\tinystr.cpp
# End Source File
# Begin Source File

SOURCE=..\..\tinyxml\tinyxml.cpp
# End Source File
# Begin Source File

SOURCE=..\..\tinyxml\tinyxmlerror.cpp
# End Source File
# Begin Source File

SOURCE=..\..\tinyxml\tinyxmlparser.cpp
# End Source File
# End Group
# Begin Group "zip_util"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\zip_utils\unzip.cpp
# End Source File
# Begin Source File

SOURCE=..\..\zip_utils\zip.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\base64.cpp
# End Source File
# Begin Source File

SOURCE=.\CrashSender.cpp
# End Source File
# Begin Source File

SOURCE=.\CrashSender.rc
# End Source File
# Begin Source File

SOURCE=.\DetailDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\dnsresolver.cpp
# End Source File
# Begin Source File

SOURCE=.\httpsend.cpp
# End Source File
# Begin Source File

SOURCE=.\MailMsg.cpp
# End Source File
# Begin Source File

SOURCE=.\MainDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ProgressDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SenderThread.cpp
# End Source File
# Begin Source File

SOURCE=.\smtpclient.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\crashrpt\src\Utility.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "tinyxmlh"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\tinyxml\tinystr.h
# End Source File
# Begin Source File

SOURCE=..\..\tinyxml\tinyxml.h
# End Source File
# End Group
# Begin Group "zip_utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\zip_utils\unzip.h
# End Source File
# Begin Source File

SOURCE=..\..\zip_utils\zip.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\base64.h
# End Source File
# Begin Source File

SOURCE=.\CrashSender.h
# End Source File
# Begin Source File

SOURCE=.\DetailDlg.h
# End Source File
# Begin Source File

SOURCE=.\dns.h
# End Source File
# Begin Source File

SOURCE=.\dnsresolver.h
# End Source File
# Begin Source File

SOURCE=.\httpsend.h
# End Source File
# Begin Source File

SOURCE=.\MailMsg.h
# End Source File
# Begin Source File

SOURCE=.\MainDlg.h
# End Source File
# Begin Source File

SOURCE=.\ProgressDlg.h
# End Source File
# Begin Source File

SOURCE=.\SenderThread.h
# End Source File
# Begin Source File

SOURCE=.\smtpclient.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
