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

// File: CrashHandler.cpp
// Description: Exception handling and report generation functionality.
// Authors: mikecarruth, zexspectrum
// Date: 

#include "stdafx.h"
#include "CrashHandler.h"
#include "Utility.h"
#include "resource.h"
#include <sys/stat.h>
#include <psapi.h>
#include "base64.h"
#include "strconv.h"
#include <rtcapi.h>

#ifndef _AddressOfReturnAddress

// Taken from: http://msdn.microsoft.com/en-us/library/s975zw7k(VS.71).aspx
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

// _ReturnAddress and _AddressOfReturnAddress should be prototyped before use 
EXTERNC void * _AddressOfReturnAddress(void);
EXTERNC void * _ReturnAddress(void);

#endif 

CCrashHandler* CCrashHandler::m_pProcessCrashHandler = NULL;

CCrashHandler::CCrashHandler()
{
  m_bInitialized = FALSE;
  
  InitPrevExceptionHandlerPointers();
  m_lpfnCallback = NULL;
  memset(&m_uPriorities, 0, 3*sizeof(UINT));
  m_hDbgHelpDll = 0;
  m_MiniDumpType = MiniDumpNormal;
  m_hEvent = NULL;
  m_bAddScreenshot = FALSE;
  m_dwScreenshotFlags = 0;
}

CCrashHandler::~CCrashHandler()
{
  Destroy();
}

int CCrashHandler::Init(
  LPCTSTR lpcszAppName,
  LPCTSTR lpcszAppVersion,
  LPCTSTR lpcszCrashSenderPath,
  LPGETLOGFILE lpfnCallback, 
  LPCTSTR lpcszTo, 
  LPCTSTR lpcszSubject,
  LPCTSTR lpcszUrl,
  UINT (*puPriorities)[5],
  DWORD dwFlags,
  LPCTSTR lpcszPrivacyPolicyURL,
  LPCTSTR lpcszDebugHelpDLLPath,
  MINIDUMP_TYPE MiniDumpType)
{ 
  crSetErrorMsg(_T("Unspecified error."));
  
  // Save minidump type
  m_MiniDumpType = MiniDumpType;

  // Save user supplied callback
  m_lpfnCallback = lpfnCallback;
  
  // Get handle to the EXE module used to create this process
  HMODULE hExeModule = GetModuleHandle(NULL);
  if(hExeModule==NULL)
  {
    ATLASSERT(hExeModule!=NULL);
    crSetErrorMsg(_T("Couldn't get module handle for the executable."));
    return 1;
  }

  // Save EXE image name
  m_sImageName = Utility::GetModuleName(hExeModule);

  // Save application name
  m_sAppName = lpcszAppName;

  // If no app name provided, use the default (EXE name)
  if(m_sAppName.IsEmpty())
  {
    m_sAppName = Utility::getAppName();
  }

  // Save app version
  m_sAppVersion = lpcszAppVersion;

  // If no app version provided, use the default (EXE product version)
  if(m_sAppVersion.IsEmpty())
  {
    m_sAppVersion = Utility::GetProductVersion(m_sImageName);
  }

  // Save URL to send reports via HTTP
  if(lpcszUrl!=NULL)
  {
    m_sUrl = CString(lpcszUrl);
  }

  // Save Email recipient address
  m_sTo = lpcszTo;

  // Save E-mail subject
  m_sSubject = lpcszSubject;

  // If the subject is empty...
  if(m_sSubject.IsEmpty())
  {
    // Generate the default subject
    m_sSubject.Format(_T("%s %s Error Report"), m_sAppName, 
      m_sAppVersion.IsEmpty()?_T("[unknown_ver]"):m_sAppVersion);
  }

  // Save report sending priorities
  if(puPriorities!=NULL)
    memcpy(&m_uPriorities, puPriorities, 3*sizeof(UINT));
  else
    memset(&m_uPriorities, 0, 3*sizeof(UINT));

  // Save privacy policy URL (if exists)
  if(lpcszPrivacyPolicyURL!=NULL)
    m_sPrivacyPolicyURL = lpcszPrivacyPolicyURL;

  // Get the name of CrashRpt DLL
  LPTSTR pszCrashRptModule = NULL;

#ifndef CRASHRPT_LIB
  #ifdef _DEBUG
    pszCrashRptModule = _T("CrashRptd.dll");
  #else
    pszCrashRptModule = _T("CrashRpt.dll");
  #endif //_DEBUG
#else //!CRASHRPT_LIB
  pszCrashRptModule = NULL;
#endif

  // Get handle to the CrashRpt module that is loaded by the current process
  HMODULE hCrashRptModule = GetModuleHandle(pszCrashRptModule);
  if(hCrashRptModule==NULL)
  {
    ATLASSERT(hCrashRptModule!=NULL);
    crSetErrorMsg(_T("Couldn't get handle to CrashRpt.dll."));
    return 1;
  }  
  
  // Save path to CrashSender.exe
  if(lpcszCrashSenderPath==NULL)
  {
    // By default assume that CrashSender.exe is located in the same dir as CrashRpt.dll
    m_sPathToCrashSender = Utility::GetModulePath(hCrashRptModule);   
  }
  else
  {
    // Save user-specified path
    m_sPathToCrashSender = CString(lpcszCrashSenderPath);    
  }

  // Remove ending backslash if any
  if(m_sPathToCrashSender.Right(1)!='\\')
      m_sPathToCrashSender+="\\";

  // Look for crashrpt_lang.ini in the same folder as CrashSender.exe
  CString sININame = m_sPathToCrashSender + _T("crashrpt_lang.ini");
  CString sLangFileVer = Utility::GetINIString(sININame, _T("Settings"), _T("CrashRptVersion"));
  int lang_file_ver = _ttoi(sLangFileVer);
  if(lang_file_ver!=CRASHRPT_VER)
  {
    ATLASSERT(lang_file_ver==CRASHRPT_VER);
    crSetErrorMsg(_T("Missing language INI file or wrong file version."));
    return 1; // Language INI file has wrong version!
  }

  // Get CrashSender EXE name
  CString sCrashSenderName;

#ifdef _DEBUG
  sCrashSenderName = _T("CrashSenderd.exe");
#else
  sCrashSenderName = _T("CrashSender.exe");
#endif //_DEBUG

  m_sPathToCrashSender+=sCrashSenderName;   

  // Check that CrashSender.exe file exists
  HANDLE hFile = CreateFile(m_sPathToCrashSender, FILE_GENERIC_READ, 
    FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);  
  if(hFile==INVALID_HANDLE_VALUE)
  {
    ATLASSERT(hFile!=INVALID_HANDLE_VALUE);
    crSetErrorMsg(_T("Couldn't locate CrashSender.exe in specified directory."));
    return 1; // CrashSender not found!
  }
  CloseHandle(hFile);

  if(lpcszDebugHelpDLLPath==NULL)
  {
    // By default assume that debughlp.dll is located in the same dir as CrashRpt.dll
    m_sPathToDebugHelpDll = Utility::GetModulePath(hCrashRptModule);   
  }
  else
    m_sPathToDebugHelpDll = CString(lpcszDebugHelpDLLPath);    

  if(m_sPathToDebugHelpDll.Right(1)!='\\')
    m_sPathToDebugHelpDll+="\\";

  const CString sDebugHelpDLL_name = "dbghelp.dll";
  m_sPathToDebugHelpDll+=sDebugHelpDLL_name;

  m_hDbgHelpDll = LoadLibrary(m_sPathToDebugHelpDll);
  if(!m_hDbgHelpDll)
  {
    //try again ... fallback to dbghelp.dll in path
    m_hDbgHelpDll = LoadLibrary(sDebugHelpDLL_name);
    if(!m_hDbgHelpDll)
    {
      ATLASSERT(m_hDbgHelpDll);
      crSetErrorMsg(_T("Couldn't load dbghelp.dll."));      
      return 1;
    }
  }

  FreeLibrary(m_hDbgHelpDll);

  // Generate unique GUID for this crash report.
  if(0!=Utility::GenerateGUID(m_sCrashGUID))
  {
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't generate crash GUID."));
    return 1; 
  }

  // Create event that will be used to synchronize with CrashSender.exe process
  CString sEventName;
  sEventName.Format(_T("Local\\CrashRptEvent_%s"), m_sCrashGUID);
  m_hEvent = CreateEvent(NULL, FALSE, FALSE, sEventName);
  if(m_hEvent==NULL)
  {
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't create synchronization event."));
    return 1; 
  }

  // Get operating system friendly name.
  if(0!=Utility::GetOSFriendlyName(m_sOSName))
  {
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't get operating system's friendly name."));
    return 1; 
  }

  // Create %LOCAL_APPDATA%\CrashRpt\UnsavedCrashReports\AppName_AppVer folder.
  CString sLocalAppDataFolder;
  DWORD dwCSIDL = CSIDL_LOCAL_APPDATA;
  Utility::GetSpecialFolder(dwCSIDL, sLocalAppDataFolder);
  m_sUnsentCrashReportsFolder.Format(_T("%s\\CrashRpt\\UnsentCrashReports\\%s_%s"), 
    sLocalAppDataFolder, m_sAppName, m_sAppVersion);
  BOOL bCreateDir = Utility::CreateFolder(m_sUnsentCrashReportsFolder);
  if(!bCreateDir)
  {
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't create crash report directory."));
    return 1; 
  }
    
  // Save the name of the folder we will save this crash report (if occur)
  m_sReportFolderName = m_sUnsentCrashReportsFolder + _T("\\") + m_sCrashGUID;

  // Set exception handlers with initial values (NULLs)
  InitPrevExceptionHandlerPointers();
   
  // Set exception handlers that work on per-process basis
  int nSetProcessHandlers = SetProcessExceptionHandlers(dwFlags);   
  if(nSetProcessHandlers!=0)
  {
    ATLASSERT(nSetProcessHandlers==0);
    crSetErrorMsg(_T("Couldn't set C++ exception handlers for current process."));
    return 1;
  }

  // Set exception handlers that work on per-thread basis
  int nSetThreadHandlers = SetThreadExceptionHandlers(dwFlags);
  if(nSetThreadHandlers!=0)
  {
    ATLASSERT(nSetThreadHandlers==0);
    crSetErrorMsg(_T("Couldn't set C++ exception handlers for main execution thread."));
    return 1;
  }
  
  // Associate this handler with the caller process
  m_pProcessCrashHandler =  this;
    
  // OK.
  m_bInitialized = TRUE;
  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::Destroy()
{
  crSetErrorMsg(_T("Unspecified error."));

  if(!m_bInitialized)
  {
    crSetErrorMsg(_T("Can't destroy not initialized handler."));
    return 1;
  }  

  // Reset exception callback
  if (m_oldSehHandler)
    SetUnhandledExceptionFilter(m_oldSehHandler);

  m_oldSehHandler = NULL;

  // All installed per-thread C++ exception handlers should be uninstalled 
  // using crUninstallFromCurrentThread() before calling Destroy()

  {
    CAutoLock lock(&m_csThreadExceptionHandlers);
    ATLASSERT(m_ThreadExceptionHandlers.size()==0);          
  }

  m_pProcessCrashHandler = NULL;

  // OK.
  m_bInitialized = FALSE;
  crSetErrorMsg(_T("Success."));
  return 0;
}


// Sets internal pointers to previously used exception handlers to NULL
void CCrashHandler::InitPrevExceptionHandlerPointers()
{
  m_oldSehHandler = NULL;

#if _MSC_VER>=1300
  m_prevPurec = NULL;
  m_prevNewHandler = NULL;
#endif

#if _MSC_VER>=1300 && _MSC_VER<1400    
   m_prevSec = NULL;
#endif

#if _MSC_VER>=1400
  m_prevInvpar = NULL;
#endif  

  m_prevSigABRT = NULL;  
  m_prevSigINT = NULL;  
  m_prevSigTERM = NULL;
}

CCrashHandler* CCrashHandler::GetCurrentProcessCrashHandler()
{  
  return m_pProcessCrashHandler;
}

int CCrashHandler::SetProcessExceptionHandlers(DWORD dwFlags)
{
  crSetErrorMsg(_T("Unspecified error."));

  // If 0 is specified as dwFlags, assume all handlers should be
  // installed
  if((dwFlags&0x1FF)==0)
    dwFlags |= 0x1FFF;
  
  if(dwFlags&CR_INST_STRUCTURED_EXCEPTION_HANDLER)
  {
    // Install structured exception handler
    m_oldSehHandler = SetUnhandledExceptionFilter(SehHandler);
  }

  _set_error_mode(_OUT_TO_STDERR);

#if _MSC_VER>=1300
  if(dwFlags&CR_INST_PURE_CALL_HANDLER)
  {
    // Catch pure virtual function calls.
    // Because there is one _purecall_handler for the whole process, 
    // calling this function immediately impacts all threads. The last 
    // caller on any thread sets the handler. 
    // http://msdn.microsoft.com/en-us/library/t296ys27.aspx
    m_prevPurec = _set_purecall_handler(PureCallHandler);    
  }

  if(dwFlags&CR_INST_NEW_OPERATOR_ERROR_HANDLER)
  {
    // Catch new operator memory allocation exceptions
    _set_new_mode(1); // Force malloc() to call new handler too
    m_prevNewHandler = _set_new_handler(NewHandler);
  }
#endif

#if _MSC_VER>=1400
  if(dwFlags&CR_INST_INVALID_PARAMETER_HANDLER)
  {
    // Catch invalid parameter exceptions.
    m_prevInvpar = _set_invalid_parameter_handler(InvalidParameterHandler); 
  }
#endif


#if _MSC_VER>=1300 && _MSC_VER<1400    
  if(dwFlags&CR_INST_SECURITY_ERROR_HANDLER)
  {
    // Catch buffer overrun exceptions
    // The _set_security_error_handler is deprecated in VC8 C++ run time library
    m_prevSec = _set_security_error_handler(SecurityHandler);
  }
#endif

   // Set up C++ signal handlers
  
  
  if(dwFlags&CR_INST_SIGABRT_HANDLER)
  {
#if _MSC_VER>=1400  
  _set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
#endif
  // Catch an abnormal program termination
  m_prevSigABRT = signal(SIGABRT, SigabrtHandler);  
  }
   
  if(dwFlags&CR_INST_SIGILL_HANDLER)
  {
    // Catch illegal instruction handler
    m_prevSigINT = signal(SIGINT, SigintHandler);     
  }
  
  if(dwFlags&CR_INST_TERMINATE_HANDLER)
  {
    // Catch a termination request
    m_prevSigTERM = signal(SIGTERM, SigtermHandler);          
  }

  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::UnSetProcessExceptionHandlers()
{
  crSetErrorMsg(_T("Unspecified error."));

  // Unset all previously set handlers

#if _MSC_VER>=1300
  if(m_prevPurec!=NULL)
    _set_purecall_handler(m_prevPurec);

  if(m_prevNewHandler!=NULL)
    _set_new_handler(m_prevNewHandler);
#endif

#if _MSC_VER>=1400
  if(m_prevInvpar!=NULL)
    _set_invalid_parameter_handler(m_prevInvpar);
#endif //_MSC_VER>=1400  

#if _MSC_VER>=1300 && _MSC_VER<1400    
  if(m_prevSec!=NULL)
    _set_security_error_handler(m_prevSec);
#endif //_MSC_VER<1400
     
  if(m_prevSigABRT!=NULL)
    signal(SIGABRT, m_prevSigABRT);  
  
  if(m_prevSigINT!=NULL)
    signal(SIGINT, m_prevSigINT);     

  if(m_prevSigTERM!=NULL)
   signal(SIGTERM, m_prevSigTERM);    

  crSetErrorMsg(_T("Success."));
  return 0;
}

// Installs C++ exception handlers that function on per-thread basis
int CCrashHandler::SetThreadExceptionHandlers(DWORD dwFlags)
{
  crSetErrorMsg(_T("Unspecified error."));

  // If 0 is specified as dwFlags, assume all available exception handlers should be
  // installed  
  if((dwFlags&0x1FFF)==0)
    dwFlags |= 0x1FFF;

  DWORD dwThreadId = GetCurrentThreadId();

  CAutoLock lock(&m_csThreadExceptionHandlers);

  std::map<DWORD, ThreadExceptionHandlers>::iterator it = 
    m_ThreadExceptionHandlers.find(dwThreadId);

  if(it!=m_ThreadExceptionHandlers.end())
  {
    // handlers are already set for the thread
    ATLASSERT(0);
    crSetErrorMsg(_T("Can't install handlers for current thread twice."));
    return 1; // failed
  }
  
  ThreadExceptionHandlers handlers;

  if(dwFlags&CR_INST_TERMINATE_HANDLER)
  {
    // Catch terminate() calls. 
    // In a multithreaded environment, terminate functions are maintained 
    // separately for each thread. Each new thread needs to install its own 
    // terminate function. Thus, each thread is in charge of its own termination handling.
    // http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
    handlers.m_prevTerm = set_terminate(TerminateHandler);       
  }

  if(dwFlags&CR_INST_UNEXPECTED_HANDLER)
  {
    // Catch unexpected() calls.
    // In a multithreaded environment, unexpected functions are maintained 
    // separately for each thread. Each new thread needs to install its own 
    // unexpected function. Thus, each thread is in charge of its own unexpected handling.
    // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
    handlers.m_prevUnexp = set_unexpected(UnexpectedHandler);    
  }

  if(dwFlags&CR_INST_SIGFPE_HANDLER)
  {
    // Catch a floating point error
    typedef void (*sigh)(int);
    handlers.m_prevSigFPE = signal(SIGFPE, (sigh)SigfpeHandler);     
  }

  
  if(dwFlags&CR_INST_SIGILL_HANDLER)
  {
    // Catch an illegal instruction
    handlers.m_prevSigILL = signal(SIGILL, SigillHandler);     
  }

  if(dwFlags&CR_INST_SIGSEGV_HANDLER)
  {
    // Catch illegal storage access errors
    handlers.m_prevSigSEGV = signal(SIGSEGV, SigsegvHandler);   
  }

  // Insert the structure to the list of handlers  
  m_ThreadExceptionHandlers[dwThreadId] = handlers;

  // OK.
  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::UnSetThreadExceptionHandlers()
{
  crSetErrorMsg(_T("Unspecified error."));

  DWORD dwThreadId = GetCurrentThreadId();

  CAutoLock lock(&m_csThreadExceptionHandlers);

  std::map<DWORD, ThreadExceptionHandlers>::iterator it = 
    m_ThreadExceptionHandlers.find(dwThreadId);

  if(it==m_ThreadExceptionHandlers.end())
  {
    // No exception handlers were installed for the caller thread?
    ATLASSERT(0);
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current thread."));
    return 1;
  }
  
  ThreadExceptionHandlers* handlers = &(it->second);

  if(handlers->m_prevTerm!=NULL)
    set_terminate(handlers->m_prevTerm);

  if(handlers->m_prevUnexp!=NULL)
    set_unexpected(handlers->m_prevUnexp);

  if(handlers->m_prevSigFPE!=NULL)
    signal(SIGFPE, handlers->m_prevSigFPE);     

  if(handlers->m_prevSigILL!=NULL)
    signal(SIGINT, handlers->m_prevSigILL);     

  if(handlers->m_prevSigSEGV!=NULL)
    signal(SIGSEGV, handlers->m_prevSigSEGV); 

  // Remove from the list
  m_ThreadExceptionHandlers.erase(it);

  // OK.
  crSetErrorMsg(_T("Success."));
  return 0;
}


int CCrashHandler::AddFile(LPCTSTR pszFile, LPCTSTR pszDestFile, LPCTSTR pszDesc, DWORD dwFlags)
{
  crSetErrorMsg(_T("Unspecified error."));

  // make sure the file exist
  struct _stat st;
  int result = _tstat(pszFile, &st);

  if (result!=0 && (dwFlags&CR_AF_MISSING_FILE_OK)==0)
  {
   ATLASSERT(0);
   crSetErrorMsg(_T("Couldn't stat file. File may not exist."));
   return 1;
  }

  // Add file to file list.
  FileItem fi;
  fi.m_sDescription = pszDesc;
  fi.m_sFileName = pszFile;
  fi.m_bMakeCopy = (dwFlags&CR_AF_MAKE_FILE_COPY)!=0;
  if(pszDestFile!=NULL)
    m_files[pszDestFile] = fi;
  else
  {
    CString sDestFile = pszFile;
    int pos = -1;
    sDestFile.Replace('/', '\\');
    pos = sDestFile.ReverseFind('\\');
    if(pos!=-1)
      sDestFile = sDestFile.Mid(pos+1);

    m_files[sDestFile] = fi;
  }

  // OK.
  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::AddProperty(CString sPropName, CString sPropValue)
{
  crSetErrorMsg(_T("Unspecified error."));

  if(sPropName.IsEmpty())
  {
    crSetErrorMsg(_T("Invalid property name specified."));
    return 1;
  }

  m_props[sPropName] = sPropValue;

  // OK.
  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::AddScreenshot(DWORD dwFlags)
{  
  m_bAddScreenshot = TRUE;
  m_dwScreenshotFlags = dwFlags;
  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::GenerateErrorReport(
  PCR_EXCEPTION_INFO pExceptionInfo)
{  
  crSetErrorMsg(_T("Unspecified error."));

  /* Validate input parameters */
  if(pExceptionInfo==NULL)
  {
    crSetErrorMsg(_T("Exception info is NULL."));
    return 1;
  }

  /* Collect miscellaneous crash info */

  CollectMiscCrashInfo();

  /* Let client add application-specific files / desktop screenshot 
     to report via the crash callback function. */

  if (m_lpfnCallback!=NULL && m_lpfnCallback(NULL)==FALSE)
  {
    crSetErrorMsg(_T("The operation was cancelled by client application."));
    return 2;
  }
  
  /* Get exception pointers if not provided. */
  if(pExceptionInfo->pexcptrs==NULL)
  {
    GetExceptionPointers(pExceptionInfo->code, &pExceptionInfo->pexcptrs);
  }
  
  /* Create directory for the error report. */
  
  BOOL bCreateDir = CreateDirectory(m_sReportFolderName, NULL);
  if(!bCreateDir)
  {    
    ATLASSERT(bCreateDir);
    CString szCaption;
    szCaption.Format(_T("%s has stopped working"), Utility::getAppName());
    CString szMessage;
    // Try notify user about crash using message box.
    szMessage.Format(_T("Couldn't save error report."));
    MessageBox(NULL, szMessage, szCaption, MB_OK|MB_ICONERROR);    
    return 1; // Failed to create directory
  }

  /* Create crash minidump file. */

  CString sFileName;
  sFileName.Format(_T("%s\\crashdump.dmp"), m_sReportFolderName);
  AddFile(sFileName, NULL, _T("Crash Dump"), CR_AF_MISSING_FILE_OK);      
      
  // Create crash report descriptor file in XML format. 
  
  sFileName.Format(_T("%s\\crashrpt.xml"), m_sReportFolderName);
  AddFile(sFileName, NULL, _T("Crash Log"), CR_AF_MISSING_FILE_OK);        
  int result = GenerateCrashDescriptorXML(sFileName.GetBuffer(0), pExceptionInfo);
  ATLASSERT(result==0);
  
  // Write internal crash info to file. This info is required by 
  // CrashSender.exe only and will not be sent to developer. 
  
  sFileName = m_sReportFolderName + _T("\\~CrashRptInternal.xml");
  result = CreateInternalCrashInfoFile(sFileName, pExceptionInfo->pexcptrs);
  ATLASSERT(result==0);
  SetFileAttributes(sFileName, FILE_ATTRIBUTE_HIDDEN);

  // Launch the CrashSender process that would take dekstop screenshot, 
  // copy user-specified files to the error report folder, create minidump, 
  // notify user about crash, compress the report into ZIP archive and send 
  // the error report 
    
  result = LaunchCrashSender(sFileName);
  if(result!=0)
  {
    ATLASSERT(result==0);
    crSetErrorMsg(_T("Error launching CrashSender.exe"));
    
    // Failed to launch crash sender process.
    // Try notify user about crash using message box.
    CString szCaption;
    szCaption.Format(_T("%s has stopped working"), Utility::getAppName());
    CString szMessage;
    szMessage.Format(_T("Error launching CrashSender.exe"));
    MessageBox(NULL, szMessage, szCaption, MB_OK|MB_ICONERROR);    
    return 3;
  }
  
  crSetErrorMsg(_T("Success."));
  return 0; 
}

void CCrashHandler::GetExceptionPointers(DWORD dwExceptionCode, 
  EXCEPTION_POINTERS** ppExceptionPointers)
{
  // The following code was taken from VC++ 8.0 CRT (invarg.c: line 104)
  
  EXCEPTION_RECORD ExceptionRecord;
  CONTEXT ContextRecord;
  memset(&ContextRecord, 0, sizeof(CONTEXT));
  
#ifdef _X86_

  __asm {
      mov dword ptr [ContextRecord.Eax], eax
      mov dword ptr [ContextRecord.Ecx], ecx
      mov dword ptr [ContextRecord.Edx], edx
      mov dword ptr [ContextRecord.Ebx], ebx
      mov dword ptr [ContextRecord.Esi], esi
      mov dword ptr [ContextRecord.Edi], edi
      mov word ptr [ContextRecord.SegSs], ss
      mov word ptr [ContextRecord.SegCs], cs
      mov word ptr [ContextRecord.SegDs], ds
      mov word ptr [ContextRecord.SegEs], es
      mov word ptr [ContextRecord.SegFs], fs
      mov word ptr [ContextRecord.SegGs], gs
      pushfd
      pop [ContextRecord.EFlags]
  }

  ContextRecord.ContextFlags = CONTEXT_CONTROL;
#pragma warning(push)
#pragma warning(disable:4311)
  ContextRecord.Eip = (ULONG)_ReturnAddress();
  ContextRecord.Esp = (ULONG)_AddressOfReturnAddress();
#pragma warning(pop)
  ContextRecord.Ebp = *((ULONG *)_AddressOfReturnAddress()-1);


#elif defined (_IA64_) || defined (_AMD64_)

  /* Need to fill up the Context in IA64 and AMD64. */
  RtlCaptureContext(&ContextRecord);

#else  /* defined (_IA64_) || defined (_AMD64_) */

  ZeroMemory(&ContextRecord, sizeof(ContextRecord));

#endif  /* defined (_IA64_) || defined (_AMD64_) */

  ZeroMemory(&ExceptionRecord, sizeof(EXCEPTION_RECORD));

  ExceptionRecord.ExceptionCode = dwExceptionCode;
  ExceptionRecord.ExceptionAddress = _ReturnAddress();

  ///
  
  EXCEPTION_RECORD* pExceptionRecord = new EXCEPTION_RECORD;
  memcpy(pExceptionRecord, &ExceptionRecord, sizeof(EXCEPTION_RECORD));
  CONTEXT* pContextRecord = new CONTEXT;
  memcpy(pContextRecord, &ContextRecord, sizeof(CONTEXT));

  *ppExceptionPointers = new EXCEPTION_POINTERS;
  (*ppExceptionPointers)->ExceptionRecord = pExceptionRecord;
  (*ppExceptionPointers)->ContextRecord = pContextRecord;  
}

void CCrashHandler::CollectMiscCrashInfo()
{  
  // Get crash time
  Utility::GetSystemTimeUTC(m_sCrashTime);

  // Get number of GUI resources in use
  HANDLE hCurProcess = GetCurrentProcess();
  m_dwGuiResources = GetGuiResources(hCurProcess, GR_GDIOBJECTS);
  
  // Determine if GetProcessHandleCount function available
  typedef BOOL (WINAPI *LPGETPROCESSHANDLECOUNT)(HANDLE, PDWORD);
  HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
  LPGETPROCESSHANDLECOUNT pfnGetProcessHandleCount = 
    (LPGETPROCESSHANDLECOUNT)GetProcAddress(hKernel32, "GetProcessHandleCount");
  if(pfnGetProcessHandleCount!=NULL)
  {    
    // Get count of opened handles
    DWORD dwHandleCount = 0;
    BOOL bGetHandleCount = GetProcessHandleCount(hCurProcess, &dwHandleCount);
    if(bGetHandleCount)
      m_dwProcessHandleCount = dwHandleCount;
    else
      m_dwProcessHandleCount = 0;
  }

  // Get memory usage info
  PROCESS_MEMORY_COUNTERS meminfo;
  BOOL bGetMemInfo = GetProcessMemoryInfo(hCurProcess, &meminfo, 
    sizeof(PROCESS_MEMORY_COUNTERS));
  if(bGetMemInfo)
  {    
    CString sMemUsage;
#ifdef _WIN64
    sMemUsage.Format(_T("%I64u"), meminfo.WorkingSetSize/1024);
#else
    sMemUsage.Format(_T("%lu"), meminfo.WorkingSetSize/1024);
#endif 
    m_sMemUsage = sMemUsage;
  }
}

int CCrashHandler::GenerateCrashDescriptorXML(LPTSTR pszFileName,          
     PCR_EXCEPTION_INFO pExceptionInfo)
{
  crSetErrorMsg(_T("Unspecified error."));

  strconv_t strconv;  
  FILE* f = NULL;

#if _MSC_VER>=1400
  _tfopen_s(&f, pszFileName, _T("wt"));
#else
  f = _tfopen(pszFileName, _T("wt"));
#endif
  
  if(f==NULL)
    return 1; // Couldn't create file

  // Add <?xml version="1.0" encoding="utf-8" ?> element
  fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");

  // Add root element
  fprintf(f, "<CrashRpt version=\"%d\">\n", CRASHRPT_VER);
  
  // Write crash GUID
  fprintf(f, "  <CrashGUID>%s</CrashGUID>\n", 
    strconv.t2utf8(_repxrch(m_sCrashGUID.GetBuffer(0))));

  // Write application name 
  fprintf(f, "  <AppName>%s</AppName>\n", 
    strconv.t2utf8(_repxrch(m_sAppName.GetBuffer(0))));

  // Write application version 
  fprintf(f, "  <AppVersion>%s</AppVersion>\n", 
    strconv.t2utf8(_repxrch(m_sAppVersion.GetBuffer(0))));
  
  // Write EXE image name
  fprintf(f, "  <ImageName>%s</ImageName>\n", 
    strconv.t2utf8(_repxrch(m_sImageName.GetBuffer(0))));

  // Write operating system friendly name  
  fprintf(f, "  <OperatingSystem>%s</OperatingSystem>\n", 
    strconv.t2utf8(_repxrch(m_sOSName.GetBuffer(0))));
  
  // Write system time in UTC format
  fprintf(f, "  <SystemTimeUTC>%s</SystemTimeUTC>\n", 
    strconv.t2utf8(_repxrch(m_sCrashTime.GetBuffer(0))));
  
  // Write exception type
  fprintf(f, "  <ExceptionType>%d</ExceptionType>\n", 
    pExceptionInfo->exctype);

  if(pExceptionInfo->exctype==CR_WIN32_STRUCTURED_EXCEPTION)
  {
    // Write exception code
    fprintf(f, "  <ExceptionCode>0x%X</ExceptionCode>\n", 
      pExceptionInfo->pexcptrs->ExceptionRecord->ExceptionCode);
  }

  if(pExceptionInfo->exctype==CR_CPP_SIGFPE)
  {
    // Write FPE exception subcode
    fprintf(f, "  <FPESubcode>%d</FPESubcode>\n", 
      pExceptionInfo->fpe_subcode);
  }

#if _MSC_VER>=1400
  if(pExceptionInfo->exctype==CR_CPP_INVALID_PARAMETER)
  {
    if(pExceptionInfo->expression!=NULL)
    {
      // Write expression      
      fprintf(f, "  <InvParamExpression>%s</InvParamExpression>\n", 
        strconv.w2utf8(_repxrch(pExceptionInfo->expression)));
    }

    if(pExceptionInfo->function!=NULL)
    {
      // Write function      
      fprintf(f, "  <InvParamFunction>%s</InvParamFunction>\n", 
        strconv.t2utf8(_repxrch(pExceptionInfo->function)));
    }

    if(pExceptionInfo->file!=NULL)
    {
      // Write file
      fprintf(f, "  <InvParamFile>%s</InvParamFile>\n", 
        strconv.w2utf8(_repxrch(pExceptionInfo->file)));
    }
    
    // Write line
    fprintf(f, "  <InvParamLine>%d</InvParamLine>\n", 
      pExceptionInfo->line);
  }
#endif 

  // Write the number of GUI resources in use
  fprintf(f, "  <GUIResourceCount>%lu</GUIResourceCount>\n", 
      m_dwGuiResources);

  // Write count of open handles that belong to current process
  fprintf(f, "  <OpenHandleCount>%lu</OpenHandleCount>\n", 
    m_dwProcessHandleCount);  

  // Write memory usage info
  fprintf(f, "  <MemoryUsageKbytes>%s</MemoryUsageKbytes>\n", 
    strconv.t2utf8(_repxrch(m_sMemUsage.GetBuffer(0))));  

  // Write list of custom user-added properties
  fprintf(f, "  <CustomProps>\n");
  
  std::map<CString, CString>::iterator pit = m_props.begin();
  unsigned i;
  for (i = 0; i < m_props.size(); i++, pit++)
  {    
    CString sPropName = pit->first;
    CString sPropValue = pit->second;

    fprintf(f, "    <Prop name=\"%s\" value=\"%s\"/>\n",
      strconv.t2utf8(_repxrch(sPropName)), strconv.t2utf8(_repxrch(sPropValue)));
  }

  fprintf(f, "  </CustomProps>\n");
  
  // Write list of files that present in this crash report

  fprintf(f, "  <FileItems>\n");
  
  std::map<CString, FileItem>::iterator cur = m_files.begin();
  for (i = 0; i < m_files.size(); i++, cur++)
  {    
    CString sDestFile = (*cur).first;

    int pos = -1;
    sDestFile.Replace('/', '\\');
    pos = sDestFile.ReverseFind('\\');
    if(pos!=-1)
      sDestFile = sDestFile.Mid(pos+1);

    FileItem& fi = cur->second;

    fprintf(f, "    <FileItem name=\"%s\" description=\"%s\" />\n",
      strconv.t2utf8(_repxrch(sDestFile)), strconv.t2utf8(_repxrch(fi.m_sDescription)));  
  }

  fprintf(f, "  </FileItems>\n");

  fprintf(f, "</CrashRpt>\n");

  fclose(f);  

  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::CreateInternalCrashInfoFile(CString sFileName, EXCEPTION_POINTERS* pExInfo)
{
  crSetErrorMsg(_T("Unspecified error."));
  
  strconv_t strconv;  

  DWORD dwProcessId = GetCurrentProcessId();
  DWORD dwThreadId = GetCurrentThreadId();

  FILE* f = NULL;

#if _MSC_VER>=1400
  _tfopen_s(&f, sFileName, _T("wt"));
#else
  f = _tfopen(sFileName, _T("wt"));
#endif
  
  if(f==NULL)
  {
    crSetErrorMsg(_T("Couldn't create internal crash info file."));
    return 1; // Couldn't create file
  }
  
  // Add <?xml version="1.0" encoding="utf-8" ?> element
  fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");

  // Add root element
  fprintf(f, "<CrashRptInternal version=\"%d\">\n", CRASHRPT_VER);

  // Add CrashGUID tag
  fprintf(f, "  <CrashGUID>%s</CrashGUID>\n", 
    strconv.t2utf8(_repxrch(m_sCrashGUID)));

  // Add ReportFolder tag
  fprintf(f, "  <ReportFolder>%s</ReportFolder>\n", 
    strconv.t2utf8(_repxrch(m_sReportFolderName)));

  // Add DbgHelpPath tag
  fprintf(f, "  <DbgHelpPath>%s</DbgHelpPath>\n", 
    strconv.t2utf8(_repxrch(m_sPathToDebugHelpDll)));

  // Add MinidumpType tag
  fprintf(f, "  <MinidumpType>%lu</MinidumpType>\n", m_MiniDumpType);

  // Add ProcessId tag
  fprintf(f, "  <ProcessId>%lu</ProcessId>\n", dwProcessId);

  // Add ThreadId tag
  fprintf(f, "  <ThreadId>%lu</ThreadId>\n", dwThreadId);

  // Add ExceptionPointersAddress tag
#ifdef _WIN64
  fprintf(f, "  <ExceptionPointersAddress>%I64lu</ExceptionPointersAddress>\n", pExInfo);
#else
  fprintf(f, "  <ExceptionPointersAddress>%I32u</ExceptionPointersAddress>\n", pExInfo );
#endif

  // Add EmailSubject tag
  fprintf(f, "  <EmailSubject>%s</EmailSubject>\n", 
    strconv.t2utf8(_repxrch(m_sSubject)));

  // Add EmailTo tag
  fprintf(f, "  <EmailTo>%s</EmailTo>\n", 
    strconv.t2utf8(_repxrch(m_sTo)));

  // Add Url tag
  fprintf(f, "  <Url>%s</Url>\n", 
    strconv.t2utf8(_repxrch(m_sUrl)));

  // Add PrivacyPolicyUrl tag
  fprintf(f, "  <PrivacyPolicyUrl>%s</PrivacyPolicyUrl>\n", 
    strconv.t2utf8(_repxrch(m_sPrivacyPolicyURL)));

  // Add HttpPriority tag
  fprintf(f, "  <HttpPriority>%d</HttpPriority>\n", m_uPriorities[CR_HTTP]);

  // Add SmtpPriority tag
  fprintf(f, "  <SmtpPriority>%d</SmtpPriority>\n", m_uPriorities[CR_SMTP]);

  // Add MapiPriority tag
  fprintf(f, "  <MapiPriority>%d</MapiPriority>\n", m_uPriorities[CR_SMAPI]);

  // Add AddScreenshot tag
  fprintf(f, "  <AddScreenshot>%d</AddScreenshot>\n", m_bAddScreenshot);

  // Add ScreenshotFlags tag
  fprintf(f, "  <ScreenshotFlags>%ul</ScreenshotFlags>\n", m_dwScreenshotFlags);

  // Write file items
  fprintf(f, "  <FileItems>\n");
    
  std::map<CString, FileItem>::iterator it;
  for(it=m_files.begin(); it!=m_files.end(); it++)
  {
    fprintf(f, "    <FileItem destfile=\"%s\" srcfile=\"%s\" description=\"%s\" makecopy=\"%d\" />\n",
      strconv.t2utf8(_repxrch(it->first)), 
      strconv.t2utf8(_repxrch(it->second.m_sFileName)), 
      strconv.t2utf8(_repxrch(it->second.m_sDescription)), 
      it->second.m_bMakeCopy?1:0 );    
  }

  fprintf(f, "  </FileItems>\n");
  
  fprintf(f, "</CrashRptInternal>\n");

  fclose(f);

  crSetErrorMsg(_T("Success."));
  return 0;
}

// Launches CrashSender.exe process
int CCrashHandler::LaunchCrashSender(CString sCrashInfoFileName)
{
  crSetErrorMsg(_T("Success."));
  
  /* Create CrashSender process */

  STARTUPINFO si;
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));  

  CString sArgs;
  sArgs.Format(_T("\"%s\""), sCrashInfoFileName.GetBuffer(0));
  BOOL bCreateProcess = CreateProcess(
    m_sPathToCrashSender, sArgs.GetBuffer(0), 
    NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
  if(!bCreateProcess)
  {
    ATLASSERT(bCreateProcess);
    crSetErrorMsg(_T("Error creating CrashSender process."));
    return 1;
  }

  /* Wait until CrashSender finishes with making screenshot, 
     copying files, creating minidump. */  

  WaitForSingleObject(m_hEvent, INFINITE);  

  return 0;
}

// Helper method that replaces characters restricted by XML
CString CCrashHandler::_repxrch(CString sText)
{
  CString sResult;

  sResult = sText;
  sResult.Replace(_T("\""), _T("&quot"));
  sResult.Replace(_T("'"), _T("&apos"));

  return sResult;
}

// Structured exception handler
LONG WINAPI CCrashHandler::SehHandler(PEXCEPTION_POINTERS pExceptionPtrs)
{  
  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_WIN32_STRUCTURED_EXCEPTION;
    ei.pexcptrs = pExceptionPtrs;

    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1);
}

// CRT terminate() call handler
void __cdecl CCrashHandler::TerminateHandler()
{
  // Abnormal program termination (terminate() function was called)

  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {    
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_TERMINATE_CALL;
    
    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1); 
}

// CRT unexpected() call handler
void __cdecl CCrashHandler::UnexpectedHandler()
{
  // Unexpected error (unexpected() function was called)
  
  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  { 
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_UNEXPECTED_CALL;

    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1); 
}

// CRT Pure virtual method call handler
#if _MSC_VER>=1300
void __cdecl CCrashHandler::PureCallHandler()
{
  // Pure virtual function call
    
  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {    
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_PURE_CALL;
    
    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1); 
}
#endif

// CRT buffer overrun handler. Available in CRT 7.1 only
#if _MSC_VER>=1300 && _MSC_VER<1400
void __cdecl CCrashHandler::SecurityHandler(int code, void *x)
{
  // Security error (buffer overrun).

  code;
  x;
  
  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {    
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_SECURITY_ERROR;
    
    pCrashHandler->GenerateErrorReport(&ei);
  }

  exit(1); // Terminate program 
}
#endif 

// CRT invalid parameter handler
#if _MSC_VER>=1400
void __cdecl CCrashHandler::InvalidParameterHandler(
  const wchar_t* expression, 
  const wchar_t* function, 
  const wchar_t* file, 
  unsigned int line, 
  uintptr_t pReserved)
 {
   pReserved;

   // Invalid parameter exception
   
   CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
   ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  { 
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_INVALID_PARAMETER;
    ei.expression = expression;
    ei.function = function;
    ei.file = file;
    ei.line = line;    

    pCrashHandler->GenerateErrorReport(&ei);
  }

   exit(1); // Terminate program
 }
#endif

// CRT new operator fault handler
#if _MSC_VER>=1300
int __cdecl CCrashHandler::NewHandler(size_t)
{
  // 'new' operator memory allocation exception
   
  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {     
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_NEW_OPERATOR_ERROR;
    ei.pexcptrs = NULL;    

    pCrashHandler->GenerateErrorReport(&ei);
  }

   exit(1); // Terminate program
}
#endif //_MSC_VER>=1300

// CRT SIGABRT signal handler
void CCrashHandler::SigabrtHandler(int)
{
  // Caught SIGABRT C++ signal

  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {     
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_SIGABRT;    

    pCrashHandler->GenerateErrorReport(&ei);
  }
 
  // Terminate program
  exit(1);
}

// CRT SIGFPE signal handler
void CCrashHandler::SigfpeHandler(int /*code*/, int subcode)
{
  // Floating point exception (SIGFPE)
 
  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {     
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_SIGFPE;
    ei.pexcptrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;
    ei.fpe_subcode = subcode;

    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1);
}

// CRT sigill signal handler
void CCrashHandler::SigillHandler(int)
{
  // Illegal instruction (SIGILL)

  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {    
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_SIGILL;
    
    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1);
}

// CRT sigint signal handler
void CCrashHandler::SigintHandler(int)
{
  // Interruption (SIGINT)

  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  { 
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_SIGINT;

    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1);
}

// CRT SIGSEGV signal handler
void CCrashHandler::SigsegvHandler(int)
{
  // Invalid storage access (SIGSEGV)

  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);
  
  if(pCrashHandler!=NULL)
  {     
    // Fill in exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);    
    ei.exctype = CR_CPP_SIGSEGV;
    ei.pexcptrs = (PEXCEPTION_POINTERS)_pxcptinfoptrs;
        
    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1);
}

// CRT SIGTERM signal handler
void CCrashHandler::SigtermHandler(int)
{
  // Termination request (SIGTERM)

  CCrashHandler* pCrashHandler = CCrashHandler::GetCurrentProcessCrashHandler();
  ATLASSERT(pCrashHandler!=NULL);

  if(pCrashHandler!=NULL)
  {    
    // Fill in the exception info
    CR_EXCEPTION_INFO ei;
    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
    ei.cb = sizeof(CR_EXCEPTION_INFO);
    ei.exctype = CR_CPP_SIGTERM;
    
    pCrashHandler->GenerateErrorReport(&ei);
  }

  // Terminate program
  exit(1);
}


