///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashHandler.h
//
//    Desc: CCrashHandler is the main class used by crashrpt to manage all
//          of the details associated with handling the exception and generating
//          the report.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CRASHHANDLER_H_
#define _CRASHHANDLER_H_

#include "stdafx.h"
#include <signal.h>
#include <exception>
#include "CrashRpt.h"      
#include "Utility.h"
#include "CritSec.h"

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

// Sets the last error message (for the caller thread).
int crSetErrorMsg(PTSTR pszErrorMsg);

struct FileItem
{
  CString m_sFileName;    // Path to the original file 
  CString m_sDescription; // Description
  BOOL m_bMakeCopy;       // Should we make a copy of this file on crash?
};

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
      LPGETLOGFILE lpfnCallback = NULL,           
      LPCTSTR lpcszTo = NULL,             
      LPCTSTR lpcszSubject = NULL,
      LPCTSTR lpcszUrl = NULL,
      UINT (*puPriorities)[5] = NULL,
      DWORD dwFlags = 0,
      LPCTSTR lpcszPrivacyPolicyURL = NULL,
      LPCTSTR lpcszDebugHelpDLLPath = NULL,
      MINIDUMP_TYPE MiniDumpType = MiniDumpNormal);

   int Destroy();
   
   int 
   AddFile(
      LPCTSTR lpFile,                     // File name
      LPCTSTR lpDestFile,                 // Destination file name
      LPCTSTR lpDesc,                     // File description
      DWORD dwFlags
      );

  // Adds a named text property to the report
  int AddProperty(CString sPropName, CString sPropValue);

  // Adds desktop screenshot on crash
  int AddScreenshot(DWORD dwFlags);

  // Generates error report
  int GenerateErrorReport(PCR_EXCEPTION_INFO pExceptionInfo = NULL);
     
  // Sets/unsets exception handlers for the current process
  int SetProcessExceptionHandlers(DWORD dwFlags);
  int UnSetProcessExceptionHandlers();

  // Sets/unsets exception handlers for the caller thread
  int SetThreadExceptionHandlers(DWORD dwFlags);   
  int UnSetThreadExceptionHandlers();
  
  // Returns the crash handler object if such object was 
  // created for the current process
  static CCrashHandler* GetCurrentProcessCrashHandler();

protected:
  
  // Collects current process state
  void GetExceptionPointers(DWORD dwExceptionCode, 
    EXCEPTION_POINTERS** pExceptionPointers);
  
  // Collects various information useful for crash analyzis
  void CollectMiscCrashInfo();
    
  // Creates crash descriptor XML file
  int GenerateCrashDescriptorXML(LPTSTR pszFileName, 
     PCR_EXCEPTION_INFO pExceptionInfo);

  // Creates internally used crash description file
  int CreateInternalCrashInfoFile(CString sFileName, 
    EXCEPTION_POINTERS* pExInfo);
  
  // Launches the CrashSender.exe process
  int LaunchCrashSender(CString sCrashInfoFileName);  

  // Replaces characters that are restricted in XML.
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
  CCritSec m_csThreadExceptionHandlers; // Synchronization lock for m_ThreadExceptionHandlers

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
  CString m_sUnsentCrashReportsFolder; // Folder where unsent crash reports should be saved.
  CString m_sReportFolderName;   // Folder where current crash report will be saved.
  CString m_sPrivacyPolicyURL;   // Privacy policy URL  
  HMODULE m_hDbgHelpDll;         // HANDLE to debug help DLL
  CString m_sPathToDebugHelpDll; // Path to dbghelp DLL
  MINIDUMP_TYPE m_MiniDumpType;  // Mini dump type 

  CString m_sCrashTime;          // Crash time in UTC format
  CString m_sOSName;             // Operating system name.
  DWORD m_dwGuiResources;        // Count of GUI resources in use
  DWORD m_dwProcessHandleCount;  // Count of opened handles
  CString m_sMemUsage;           // Memory usage

  BOOL m_bAddScreenshot;         // Should we make a desktop screenshot on crash?
  DWORD m_dwScreenshotFlags;     // Screenshot flags

  HANDLE m_hEvent;               // Event used to synchronize with CrashSender.exe

  BOOL m_bInitialized;           // Flag telling if this object was are initialized.
};

#endif	// !_CRASHHANDLER_H_
