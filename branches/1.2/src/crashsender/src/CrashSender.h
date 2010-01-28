// CrashSender.h
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
  EXCEPTION_POINTERS m_ExInfo;
  std::map<CString, FileItem>  m_FileItems; 

  // Gets crash info from XML
  int ParseCrashInfo(CString sCrashInfoFile);

private:

  // Gets the list of file items 
  int ParseFileList(TiXmlHandle& hRoot);

  int ParseCrashDescriptor(CString sFileName);

};

extern CCrashInfo g_CrashInfo;
