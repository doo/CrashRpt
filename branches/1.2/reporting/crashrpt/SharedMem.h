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

#pragma once
#include "stdafx.h"
#include "CritSec.h"

struct GENERIC_HEADER
{
  BYTE m_uchMagic[3]; // Magic sequence
  WORD m_wSize;       // Size of this chunk in bytes.
};

// String description.
struct STRING_DESC
{
  BYTE m_uchMagic[3]; // Magic sequence "STR"  
  WORD m_wSize;       // String data length in bytes.
};

// File item entry.
struct FILE_ITEM
{
  BYTE m_uchMagic[3]; // Magic sequence "FIL"
  WORD m_wSize;
  DWORD m_dwSrcFilePathOffs; // Path to the original file.
  DWORD m_dwDstFileNameOffs; // Name of the destination file.
  DWORD m_dwDescriptionOffs; // File description.
  BOOL  m_bMakeCopy;         // Should we make a copy of this file on crash?
};

// Registry key entry.
struct REG_KEY
{
  BYTE m_uchMagic[3];        // Magic sequence "REG"
  WORD m_wSize;
  DWORD m_dwRegKeyNameOffs;  // Registry key name.
  DWORD m_dwDstFileNameOffs; // Destination file name.
};

// User-defined property.
struct CUSTOM_PROP
{
  BYTE m_uchMagic[3];  // Magic sequence "CPR"
  WORD m_wSize;
  DWORD m_dwNameOffs;  // Property name.
  DWORD m_dwValueOffs; // Property value.
};

// Crash description. 
struct CRASH_DESCRIPTION
{  
  BYTE m_uchMagic[3];  // Magic sequence "CRD"
  WORD m_wSize;      // Size of this structure in bytes.
  DWORD m_dwTotalSize; // Total size of the whole used shared mem.
  UINT m_uFileItems;                  // Count of file item records.
  UINT m_uRegKeyEntries;              // Count of registry key entries.
  UINT m_uCustomProps;                // Count of user-defined properties.  
  DWORD m_dwInstallFlags;
  int m_nSmtpPort;            
  int m_nSmtpProxyPort;  
  UINT m_uPriorities[3];  
  MINIDUMP_TYPE m_MinidumpType;   
  DWORD m_dwScreenshotFlags;        
  DWORD m_dwUrlOffs;
  DWORD m_dwAppNameOffs;        
  DWORD m_dwAppVersionOffs;     
  DWORD m_dwLangFileNameOffs;       
  DWORD m_dwRestartCmdLineOffs; 
  DWORD m_dwEmailToOffs;     
  DWORD m_dwCrashGUIDOffs;
  DWORD m_dwUnsentCrashReportsFolderOffs;  
  DWORD m_dwPrivacyPolicyURLOffs;
  DWORD m_dwEmailSubjectOffs;
  DWORD m_dwEmailTextOffs;
  DWORD m_dwSmtpProxyServerOffs;  
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
  void DestroyView(LPBYTE pViewPtr);

private:
  
	HANDLE m_hFileMapping;		  // Memory mapped object
  DWORD m_dwAllocGranularity; // System allocation granularity  	  
	ULONG64 m_uSize;	      	  // Size of the file mapping.		  
  std::set<LPBYTE> m_aViewStartPtrs; // Base of the view of the file mapping.    
};


