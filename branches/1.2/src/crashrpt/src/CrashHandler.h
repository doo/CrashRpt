///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashHandler.h
//
//    Desc: CCrashHandler is the main class used by crashrpt to manage all
//          of the details associated with handling the exception, generating
//          the report, gathering client input, and sending the report.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CRASHHANDLER_H_
#define _CRASHHANDLER_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "crashrpt.h"      
#include <new.h>
#include <map>
#include <stdlib.h>
#include <signal.h>
#include <exception>
#include <string>
#include <dbghelp.h>

typedef std::map<CString, CString> TStrStrMap;

/* This structure contains pointer to the exception handlers for a thread.*/
struct _cpp_thread_exception_handlers
{
  _cpp_thread_exception_handlers()
  {
    m_prevTerm = NULL;
    m_prevUnexp = NULL;
    m_prevSigFPE = NULL;
    m_prevSigILL = NULL;
    m_prevSigSEGV = NULL;
  }

  terminate_handler m_prevTerm;        // Previous terminate handler   
  unexpected_handler m_prevUnexp;      // Previous unexpected handler
  void (__cdecl *m_prevSigFPE)(int);   // Previous FPE handler
  void (__cdecl *m_prevSigILL)(int);   // Previous 
  void (__cdecl *m_prevSigSEGV)(int);  // Previous illegal storage access handler
};

int crSetErrorMsg(PTSTR pszErrorMsg);

struct FileItem
{
  CString m_sFileName;    // Path to the original file 
  CString m_sDescription; // Description
  BOOL m_bMakeCopy;       // Should we make a copy of this file on crash?
};

// ===========================================================================
// CCrashHandler
// 
// See the module comment at top of file.
//
class CCrashHandler  
{
public:
	
   // Default constructor.
   CCrashHandler();

   virtual 
   ~CCrashHandler();

   int Init(
      LPCTSTR lpcszAppName = NULL,
      LPCTSTR lpcszAppVersion = NULL,
      LPCTSTR lpcszCrashSenderPath = NULL,
      LPGETLOGFILE lpfn = NULL,           
      LPCTSTR lpcszTo = NULL,             
      LPCTSTR lpcszSubject = NULL,
      LPCTSTR lpcszUrl = NULL,
      UINT (*puPriorities)[5] = NULL,
      DWORD dwFlags = 0,
      LPCTSTR lpcszPrivacyPolicyURL = NULL);

   int Destroy();
   
   int 
   AddFile(
      LPCTSTR lpFile,                     // File name
      LPCTSTR lpDestFile,                 // Destination file name
      LPCTSTR lpDesc,                     // File description
      DWORD dwFlags
      );

   int GenerateErrorReport(PCR_EXCEPTION_INFO pExceptionInfo = NULL);
     
   int SetProcessExceptionHandlers(DWORD dwFlags);
   int UnSetProcessExceptionHandlers();

   int SetThreadExceptionHandlers(DWORD dwFlags);   
   int UnSetThreadExceptionHandlers();
  
   int AddProperty(CString sPropName, CString sPropValue);

   static CCrashHandler* GetCurrentProcessCrashHandler();

protected:
  
  void GetExceptionPointers(DWORD dwExceptionCode, EXCEPTION_POINTERS** pExceptionPointers);
  void CollectMiscCrashInfo();
  int CreateMinidump(LPCTSTR pszFileName, EXCEPTION_POINTERS* pExInfo);  
  int GenerateCrashDescriptorXML(LPTSTR pszFileName, 
     PCR_EXCEPTION_INFO pExceptionInfo);
  int LaunchCrashSender(CString sErrorReportFolderName);  

  CString _repxrch(CString sText);
  
  // Sets internal pointers to exception handlers to NULL
  void InitPrevCPPExceptionHandlerPointers();

  LPTOP_LEVEL_EXCEPTION_FILTER  m_oldFilter;      // previous exception filter
      
#if _MSC_VER>=1300
  _purecall_handler m_prevPurec;   // Previous pure virtual call exception filter
  _PNH m_prevNewHandler; // Previous new operator exception filter
#endif

#if _MSC_VER>=1400
  _invalid_parameter_handler m_prevInvpar; // Previous invalid parameter exception filter  
#endif

#if _MSC_VER>=1300 && _MSC_VER<1400
  _secerr_handler_func m_prevSec; // Previous security exception filter
#endif

  void (__cdecl *m_prevSigABRT)(int); // Previous SIGABRT handler  
  void (__cdecl *m_prevSigINT)(int);  // Previous SIGINT handler
  void (__cdecl *m_prevSigTERM)(int); // Previous SIGTERM handler

  // List of exception handlers installed for threads of current process
  std::map<DWORD, _cpp_thread_exception_handlers> m_ThreadExceptionHandlers;

  LPGETLOGFILE m_lpfnCallback;   // Client crash callback.
  int m_pid;                     // Process id.
  std::map<CString, FileItem> m_files;  // Files to add.
  std::map<CString, CString> m_props;   // User-defined properties
  CString m_sTo;                 // Email:To.
  CString m_sSubject;            // Email:Subject.
  CString m_sUrl;                // URL for sending reports via HTTP.
  UINT m_uPriorities[3];         // Which way to prefer when sending crash report?
  CString m_sAppName;            // Application name.
  CString m_sAppVersion;         // Application version.
  CString m_sImageName;          // Path to client executable file.
  CString m_sPathToCrashSender;  // Path to crash sender exectuable file.  
  CString m_sCrashGUID;          // Unique ID of the crash report.
  CString m_sOSName;             // Operating system name.
  CString m_sUnsentCrashReportsFolder; // Folder where unsent crash reports should be saved.
  CString m_sPrivacyPolicyURL;   // Privacy policy URL

  CString m_sCrashTime;          // Crash time in UTC format
  DWORD m_dwGuiResources;        // Count of GUI resources in use
  DWORD m_dwProcessHandleCount;  // Count of opened handles
  CString m_sMemUsage;           // Memory usage

  BOOL m_bInitialized;
};

#endif	// !_CRASHHANDLER_H_
