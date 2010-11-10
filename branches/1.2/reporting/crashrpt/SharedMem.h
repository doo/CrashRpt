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
#include "CritSec.h"

// String description.
struct STRING_DESC
{
  BYTE m_uchMagic[3]; // Magic sequence "STR"
  DWORD m_dwOffset;   // String data offset from the beginning of the shared memory.
  WORD m_wLength;     // String data length in bytes.
};

// File item entry.
struct FILE_ITEM
{
  BYTE m_uchMagic[3]; // Magic sequence "FIL"
  STRING_DESC m_SrcFilePath; // Path to the original file.
  STRING_DESC m_DstFileName; // Name of the destination file.
  STRING_DESC m_Description; // File description.
  BOOL  m_bMakeCopy;         // Should we make a copy of this file on crash?
};

// Registry key entry.
struct REG_KEY
{
  BYTE m_uchMagic[3];        // Magic sequence "REG"
  STRING_DESC m_RegKeyName;  // Registry key name.
  STRING_DESC m_DstFileName; // Destination file name.
};

// User-defined property.
struct CUSTOM_PROP
{
  BYTE m_uchMagic[3];  // Magic sequence "CPR"
  STRING_DESC m_Name;  // Property name.
  STRING_DESC m_Value; // Property value.
};

// Crash description. 
struct CRASH_DESCRIPTION
{  
  BYTE m_uchMagic[3];  // Magic sequence "CRD"
  DWORD m_dwSize;      // Size of this structure in bytes.
  DWORD m_dwTotalSize; // Total size of the whole used shared mem.
  UINT m_uFileItems;                  // Count of file item records.
  UINT m_uRegKeyEntries;              // Count of registry key entries.
  UINT m_uCustomProps;                // Count of user-defined properties.  
  STRING_DESC m_AppName;        
  STRING_DESC m_AppVersion;     
  STRING_DESC m_LangFileName;   
  DWORD m_dwInstallFlags;
  DWORD m_dwScreenshotFlags;        
  STRING_DESC m_RestartCmdLine; 
  STRING_DESC m_EmailTo;   
  int   m_nSmtpPort;            
  STRING_DESC m_EmailSubject;
  STRING_DESC m_EmailText;
  STRING_DESC m_SmtpProxyServer;
  int m_nSmtpProxyPort;
  STRING_DESC m_Url;
  UINT m_uPriorities[3];  
  STRING_DESC m_PathToCrashSender;
  STRING_DESC m_CrashGUID;
  STRING_DESC m_UnsentCrashReportsFolder;
  STRING_DESC m_ReportFolderName;
  STRING_DESC m_PrivacyPolicyURL;
  MINIDUMP_TYPE m_MiniDumpType;      
};

// Used to share memory between CrashRpt.dll and CrashSender.exe
class CSharedMem
{
public:

	CSharedMem();  
  ~CSharedMem();  

	BOOL Init(LPCTSTR szName, BOOL bOpenExisting, ULONG64 uSize);
	BOOL Destroy();

  ULONG64 GetSize();
	LPBYTE CreateView(DWORD dwOffset, DWORD dwLength);

private:
  
	HANDLE m_hFileMapping;		  // Memory mapped object
  DWORD m_dwAllocGranularity; // System allocation granularity  	  
	ULONG64 m_uSize;	      	    // Size of the file mapping.		
  CCritSec m_csLock;
  std::map<DWORD, LPBYTE> m_aViewStartPtrs; // Base of the view of the file mapping.    
};


