/************************************************************************************* 
  This file is a part of CrashRpt library.

  Copyright (c) 2003, Michael Carruth
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:
 
   * Redistributions of source code must retain the above copyright notice, this 
     list of conditions and the following disclaimer.
 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.
 
   * Neither the name of the author nor the names of its contributors 
     may be used to endorse or promote products derived from this software without 
     specific prior written permission.
 

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

#include "stdafx.h"

// File item entry.
struct FILE_ITEM
{
  WCHAR m_szSrcFilePath[_MAX_PATH]; // Path to the original file.
  WCHAR m_szDstFileName[_MAX_PATH]; // Name of the destination file.
  WCHAR m_szDescription[1024];      // File description.
  BOOL  m_bMakeCopy;                // Should we make a copy of this file on crash?
};

// Registry key entry.
struct REG_KEY
{
  WCHAR m_szRegKeyName[4096];       // Registry key name.
  WCHAR m_szDstFileName[_MAX_PATH]; // Destination file name.
};

// User-defined property.
struct CUSTOM_PROP
{
  WCHAR m_szName[256];              // Property name.
  WCHAR m_szValue[4096];            // Property value.
};

// Crash description. 
struct CRASH_DESCRIPTION
{  
  WCHAR m_szAppName[128];              // Application name.
  WCHAR m_szAppVersion[128];           // Application version.
  WCHAR m_szLangFileName[_MAX_PATH];   // Language file to use.
  BOOL  m_bSendErrorReport;            // Should we send error report or just save it  
  BOOL  m_bStoreZIPArchives;           // Store compressed error report files as ZIP archives?
  BOOL  m_bAddScreenshot;              // Should we make a desktop screenshot on crash?
  DWORD m_dwScreenshotFlags;           // Screenshot flags.    
  BOOL  m_bAppRestart;                 // Should we restart the crashed app or not?
  WCHAR m_szRestartCmdLine[4096];      // Command line for app restart.
  HANDLE m_hEvent;                     // Event used to synchronize CrashRpt.dll with CrashSender.exe.
  WCHAR m_szEmailTo[128];              // Email recipient address.
  int   m_nSmtpPort;                   // Port for SMTP connection.
  WCHAR m_szEmailSubject[256];         // Email subject.
  WCHAR m_szEmailText[1024];           // Email message text.
  WCHAR m_szSmtpProxyServer[256];      // SMTP proxy server address.
  int m_nSmtpProxyPort;                // SMTP proxy server port.
  WCHAR m_szUrl[256];                  // URL for sending reports via HTTP.
  UINT m_uPriorities[3];               // Which method to prefer when sending crash report?
  CString m_sPathToCrashSender;  // Path to crash sender exectuable file.  
  CString m_sCrashGUID;          // Unique ID of the crash report.
  CString m_sUnsentCrashReportsFolder; // Folder where unsent crash reports should be saved.
  CString m_sReportFolderName;   // Folder where current crash report will be saved.
  CString m_sPrivacyPolicyURL;   // Privacy policy URL.  
  HMODULE m_hDbgHelpDll;         // HANDLE to debug help DLL.
  CString m_sPathToDebugHelpDll; // Path to dbghelp DLL.
  BOOL m_bGenerateMinidump;      // Should we generate minidump file?
  BOOL m_bQueueEnabled;          // Should we resend recently generated reports?
  MINIDUMP_TYPE m_MiniDumpType;  // Mini dump type. 
  BOOL m_bSilentMode;            // Do not show GUI on crash, send report silently.
  BOOL m_bHttpBinaryEncoding;    // Use HTTP uploads with binary encoding instead of the legacy (Base-64) encoding.
  UINT m_uFileItems;                  // Count of file item records.
  UINT m_uRegKeyEntries;              // Count of registry key entries.
  UINT m_uCustomProps;                // Count of user-defined properties.  
};

class CSharedMem
{
public:

  CSharedMem();
  ~CSharedMem();

private:

  CCritSec m_csLock;  
  DWORD m_dwAllocGranularity; // System allocation granularity.  
  HANDLE m_hFileMapping;   // File mapping object.
  LPVOID m_pViewStartPtr;  // View starting address.
  HANDLE m_hAccessMutex;   // Access synchronization object.
};


