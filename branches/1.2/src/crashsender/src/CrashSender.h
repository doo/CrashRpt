/************************************************************************************* 
  This file is a part of CrashRpt library.

  CrashRpt is Copyright (c) 2003, Michael Carruth
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
#include <map>
#include "tinyxml.h"
#include "dbghelp.h"

struct FileItem
{
  FileItem()
  {
    m_bMakeCopy = FALSE;
  }

  CString m_sDestFile;
  CString m_sSrcFile;
  CString m_sDesc;
  BOOL m_bMakeCopy;
};

class CCrashInfo
{
public:

  CString     m_sDbgHelpPath;
  CString     m_sCrashGUID;
  CString     m_sAppName;
  CString     m_sAppVersion;
  CString     m_sImageName;
  CString     m_sEmailSubject;
  CString     m_sEmailFrom;     
  CString     m_sEmailTo;
  CString     m_sDescription;    
  CString     m_sErrorReportDirName;
  CString     m_sUrl;
  UINT        m_uPriorities[3];
  CString     m_sPrivacyPolicyURL;
  MINIDUMP_TYPE m_MinidumpType;
  DWORD       m_dwProcessId;
  DWORD       m_dwThreadId;
  PEXCEPTION_POINTERS m_pExInfo;
  BOOL        m_bAddScreenshot;
  DWORD       m_dwScreenshotFlags;
  std::map<CString, FileItem>  m_FileItems; 

  // Gets crash info from XML file
  int ParseCrashInfo(CString sCrashInfoFile);

private:

  // Gets the list of file items 
  int ParseFileList(TiXmlHandle& hRoot);

  int ParseCrashDescriptor(CString sFileName);

};

extern CCrashInfo g_CrashInfo;
