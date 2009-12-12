///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashHandler.cpp
//
//    Desc: See CrashHandler.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CrashHandler.h"
#include "zip.h"
#include "process.h"
#include "Utility.h"
#include "resource.h"
#include <sys/stat.h>
#include "tinyxml.h"
#include <psapi.h>

#if _MSC_VER>=1300
#include <rtcapi.h>
#endif

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

// This internal structure contains the list of processes 
// that had called crInstall().
struct _crash_handlers
{
  _crash_handlers(){m_bCrashHappened=FALSE;}

  ~_crash_handlers()
  {
    // On destroy, check that client process has called crUninstall().
    ATLASSERT( m_bCrashHappened || (!m_bCrashHappened && m_map.size()==0) );    
  }

  std::map<int, CCrashHandler*> m_map; // <PID, CrashHandler> pairs
  BOOL m_bCrashHappened; // This flag is set by exception handler when crash happens
}
g_CrashHandlers;

LONG WINAPI Win32UnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionPtrs)
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

#if _MSC_VER<1300 // This is to make MSVC6.0 compiler happy
  return EXCEPTION_EXECUTE_HANDLER;
#endif
}


void __cdecl cpp_terminate_handler()
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

void __cdecl cpp_unexp_handler()
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

#if _MSC_VER>=1300
void __cdecl cpp_purecall_handler()
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

#if _MSC_VER>=1300 && _MSC_VER<1400
void __cdecl cpp_security_handler(int code, void *x)
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

#if _MSC_VER>=1400
void __cdecl cpp_invalid_parameter_handler(
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

#if _MSC_VER>=1300
int __cdecl cpp_new_handler(size_t)
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

void cpp_sigabrt_handler(int)
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

void cpp_sigfpe_handler(int /*code*/, int subcode)
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

void cpp_sigill_handler(int)
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

void cpp_sigint_handler(int)
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

void cpp_sigsegv_handler(int)
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

void cpp_sigterm_handler(int)
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

CCrashHandler::CCrashHandler()
{
  m_bInitialized = FALSE;

  m_oldFilter = NULL;
  InitPrevCPPExceptionHandlerPointers();
  m_lpfnCallback = NULL;
  m_pid = 0;
  memset(&m_uPriorities, 0, 3*sizeof(UINT));

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
  LPCTSTR lpcszPrivacyPolicyURL)
{ 
  crSetErrorMsg(_T("Unspecified error."));
  
  // save user supplied callback
  if (lpfnCallback)
    m_lpfnCallback = lpfnCallback;

  // Get handle to the EXE module used to create this process
  HMODULE hExeModule = GetModuleHandle(NULL);
  if(hExeModule==NULL)
  {
    ATLASSERT(hExeModule!=NULL);
    crSetErrorMsg(_T("Couldn't get module handle for the executable."));
    return 1;
  }

  TCHAR szExeName[_MAX_PATH];
  DWORD dwLength = GetModuleFileName(hExeModule, szExeName, _MAX_PATH);  
  if(dwLength==0)
  {
    // Couldn't get the name of EXE that was used to create current process
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't get the name of EXE that was used to create current process."));
    return 1;
  }  

  // Save EXE image name
  m_sImageName = CString(szExeName, dwLength);

  // Save application name
  m_sAppName = lpcszAppName;

  // If no app name provided, use the default (EXE name)
  if(m_sAppName.IsEmpty())
    m_sAppName = Utility::getAppName();

  // Save app version
  m_sAppVersion = lpcszAppVersion;

  // If no app version provided, use the default (EXE product version)
  if(m_sAppVersion.IsEmpty())
  {
    DWORD dwBuffSize = GetFileVersionInfoSize(szExeName, 0);
    LPBYTE pBuff = new BYTE[dwBuffSize];
    
    if(0!=GetFileVersionInfo(szExeName, 0, dwBuffSize, pBuff))
    {
      VS_FIXEDFILEINFO* fi = NULL;
      UINT uLen = 0;
      VerQueryValue(pBuff, _T("\\"), (LPVOID*)&fi, &uLen);

      WORD dwVerMajor = (WORD)(fi->dwProductVersionMS>>16);
      WORD dwVerMinor = (WORD)(fi->dwProductVersionMS&0xFF);
      WORD dwPatchLevel = (WORD)(fi->dwProductVersionLS>>16);
      WORD dwVerBuild = (WORD)(fi->dwProductVersionLS&0xFF);

      m_sAppVersion.Format(_T("%u.%u.%u.%u"), 
        dwVerMajor, dwVerMinor, dwPatchLevel, dwVerBuild);
    }

    delete [] pBuff;
  }

  if(lpcszUrl!=NULL)
    m_sUrl = CString(lpcszUrl);

  // save email info
  m_sTo = lpcszTo;

  if(m_sTo.IsEmpty() && m_sUrl.IsEmpty())
  {
    crSetErrorMsg(_T("Error report recipient's address is not defined."));
    ATLASSERT(!m_sTo.IsEmpty() || !m_sUrl.IsEmpty());
    return 1;
  }

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

  // Get handle to the CrashRpt module that is loaded by current process
  HMODULE hCrashRptModule = GetModuleHandle(pszCrashRptModule);
  if(hCrashRptModule==NULL)
  {
    ATLASSERT(hCrashRptModule!=NULL);
    crSetErrorMsg(_T("Couldn't get handle to CrashRpt.dll."));
    return 1;
  }  
  
  if(lpcszCrashSenderPath==NULL)
  {
    // By default assume that CrashSender.exe is located in the same dir as CrashRpt.dll
    m_sPathToCrashSender = Utility::GetModulePath(hCrashRptModule);   
  }
  else
    m_sPathToCrashSender = CString(lpcszCrashSenderPath);    

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

  HANDLE hFile = CreateFile(m_sPathToCrashSender, FILE_GENERIC_READ, 
    FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  
  if(hFile==INVALID_HANDLE_VALUE)
  {
    ATLASSERT(hFile!=INVALID_HANDLE_VALUE);
    crSetErrorMsg(_T("Couldn't locate CrashSender.exe in specified directory."));
    return 1; // CrashSender not found!
  }

  CloseHandle(hFile);
  
  // Generate unique GUID for this crash report.
  if(0!=Utility::GenerateGUID(m_sCrashGUID))
  {
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't generate crash GUID."));
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
  if(sLocalAppDataFolder.Right(1)!='\\')
    sLocalAppDataFolder += _T("\\");
  
  CString sCrashRptFolder = sLocalAppDataFolder+_T("CrashRpt");
  if(FALSE==CreateDirectory(sCrashRptFolder, NULL) && GetLastError()!=ERROR_ALREADY_EXISTS)
  {
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't create CrashRpt directory."));
    return 1; 
  }

  CString sUnsentCrashReportsFolder = sCrashRptFolder+_T("\\UnsentCrashReports");
  if(FALSE==CreateDirectory(sUnsentCrashReportsFolder, NULL) && GetLastError()!=ERROR_ALREADY_EXISTS)
  {
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't create UnsentCrashReports directory."));
    return 1; 
  }

  CString str = sUnsentCrashReportsFolder+_T("\\")+m_sAppName+_T("_")+m_sAppVersion;
  CString sUnsentCrashReportsFolderAppName = Utility::ReplaceInvalidCharsInFileName(str);
  if(FALSE==CreateDirectory(sUnsentCrashReportsFolderAppName, NULL) && GetLastError()!=ERROR_ALREADY_EXISTS)
  {
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't create UnsentCrashReports\\AppName directory."));
    return 1; 
  }

  m_sUnsentCrashReportsFolder = sUnsentCrashReportsFolderAppName;
    
  // Set C++ exception handlers
  InitPrevCPPExceptionHandlerPointers();
   
  int nSetProcessHandlers = SetProcessExceptionHandlers(dwFlags);   
  if(nSetProcessHandlers!=0)
  {
    ATLASSERT(nSetProcessHandlers==0);
    crSetErrorMsg(_T("Couldn't set C++ exception handlers for current process."));
    return 1;
  }

  int nSetThreadHandlers = SetThreadExceptionHandlers(dwFlags);
  if(nSetThreadHandlers!=0)
  {
    ATLASSERT(nSetThreadHandlers==0);
    crSetErrorMsg(_T("Couldn't set C++ exception handlers for main execution thread."));
    return 1;
  }

  // associate this handler with the caller process
  m_pid = _getpid();
  g_CrashHandlers.m_map[m_pid] =  this;
    
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
  if (m_oldFilter)
    SetUnhandledExceptionFilter(m_oldFilter);

  m_oldFilter = NULL;

  // All installed per-thread C++ exception handlers should be uninstalled 
  // using crUninstallFromCurrentThread() before calling Destroy()
  
  ATLASSERT(m_ThreadExceptionHandlers.size()==0);      
  m_ThreadExceptionHandlers.clear();

  std::map<int, CCrashHandler*>::iterator it = g_CrashHandlers.m_map.find(m_pid);
  if(it==g_CrashHandlers.m_map.end())
  {    
    // No such crash handler list entry
    ATLASSERT(!g_CrashHandlers.m_bCrashHappened); 
    return 1;
  }

  g_CrashHandlers.m_map.erase(it);

  // OK.
  m_bInitialized = FALSE;
  crSetErrorMsg(_T("Success."));
  return 0;
}


// Sets internal pointers to previously used exception handlers to NULL
void CCrashHandler::InitPrevCPPExceptionHandlerPointers()
{

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
  int pid = _getpid();
  std::map<int, CCrashHandler*>::iterator it = g_CrashHandlers.m_map.find(pid);
  if(it==g_CrashHandlers.m_map.end())
    return NULL; // No handler for calling process.

  return it->second;
}

int CCrashHandler::SetProcessExceptionHandlers(DWORD dwFlags)
{
  crSetErrorMsg(_T("Unspecified error."));

  // If 0 is specified as dwFlags, assume all handlers should be
  // installed
  if(dwFlags==0)
    dwFlags = 0x1FFF;
  
  if(dwFlags&CR_INST_STRUCTURED_EXCEPTION_HANDLER)
  {
    // Install structured exception handler
    m_oldFilter = SetUnhandledExceptionFilter(Win32UnhandledExceptionFilter);
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
    m_prevPurec = _set_purecall_handler(cpp_purecall_handler);    
  }

  if(dwFlags&CR_INST_NEW_OPERATOR_ERROR_HANDLER)
  {
    // Catch new operator memory allocation exceptions
    _set_new_mode(1); // Force malloc() to call new handler too
    m_prevNewHandler = _set_new_handler(cpp_new_handler);
  }
#endif

#if _MSC_VER>=1400
  if(dwFlags&CR_INST_INVALID_PARAMETER_HANDLER)
  {
    // Catch invalid parameter exceptions.
    m_prevInvpar = _set_invalid_parameter_handler(cpp_invalid_parameter_handler); 
  }
#endif


#if _MSC_VER>=1300 && _MSC_VER<1400    
  if(dwFlags&CR_INST_SECURITY_ERROR_HANDLER)
  {
    // Catch buffer overrun exceptions
    // The _set_security_error_handler is deprecated in VC8 C++ run time library
    m_prevSec = _set_security_error_handler(cpp_security_handler);
  }
#endif

   // Set up C++ signal handlers
  
  
  if(dwFlags&CR_INST_SIGABRT_HANDLER)
  {
#if _MSC_VER>=1400  
  _set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
#endif
  // Catch an abnormal program termination
  m_prevSigABRT = signal(SIGABRT, cpp_sigabrt_handler);  
  }
   
  if(dwFlags&CR_INST_SIGILL_HANDLER)
  {
    // Catch illegal instruction handler
    m_prevSigINT = signal(SIGINT, cpp_sigint_handler);     
  }
  
  if(dwFlags&CR_INST_TERMINATE_HANDLER)
  {
    // Catch a termination request
    m_prevSigTERM = signal(SIGTERM, cpp_sigterm_handler);          
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

  // If 0 is specified as dwFlags, assume all handlers should be
  // installed  
  if(dwFlags==0)
    dwFlags = 0x1FFF;

  DWORD dwThreadId = GetCurrentThreadId();

  std::map<DWORD, _cpp_thread_exception_handlers>::iterator it = 
    m_ThreadExceptionHandlers.find(dwThreadId);

  if(it!=m_ThreadExceptionHandlers.end())
  {
    // handlers are already set for the thread
    ATLASSERT(0);
    crSetErrorMsg(_T("Can't install handlers for current thread twice."));
    return 1; // failed
  }
  
  _cpp_thread_exception_handlers handlers;

  if(dwFlags&CR_INST_TERMINATE_HANDLER)
  {
    // Catch terminate() calls. 
    // In a multithreaded environment, terminate functions are maintained 
    // separately for each thread. Each new thread needs to install its own 
    // terminate function. Thus, each thread is in charge of its own termination handling.
    // http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
    handlers.m_prevTerm = set_terminate(cpp_terminate_handler);       
  }

  if(dwFlags&CR_INST_UNEXPECTED_HANDLER)
  {
    // Catch unexpected() calls.
    // In a multithreaded environment, unexpected functions are maintained 
    // separately for each thread. Each new thread needs to install its own 
    // unexpected function. Thus, each thread is in charge of its own unexpected handling.
    // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
    handlers.m_prevUnexp = set_unexpected(cpp_unexp_handler);    
  }

  if(dwFlags&CR_INST_SIGFPE_HANDLER)
  {
    // Catch a floating point error
    typedef void (*sigh)(int);
    handlers.m_prevSigFPE = signal(SIGFPE, (sigh)cpp_sigfpe_handler);     
  }

  
  if(dwFlags&CR_INST_SIGILL_HANDLER)
  {
    // Catch an illegal instruction
    handlers.m_prevSigILL = signal(SIGILL, cpp_sigill_handler);     
  }

  if(dwFlags&CR_INST_SIGSEGV_HANDLER)
  {
    // Catch illegal storage access errors
    handlers.m_prevSigSEGV = signal(SIGSEGV, cpp_sigsegv_handler);   
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

  std::map<DWORD, _cpp_thread_exception_handlers>::iterator it = 
    m_ThreadExceptionHandlers.find(dwThreadId);

  if(it==m_ThreadExceptionHandlers.end())
  {
    // no such handlers?
    ATLASSERT(0);
    crSetErrorMsg(_T("Crash handler wasn't previously installed for current thread."));
    return 1;
  }
  
  _cpp_thread_exception_handlers* handlers = &(it->second);

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
  if(sPropName.IsEmpty())
    return 1;

  m_props[sPropName] = sPropValue;
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

  /* Collect crash info */

  CollectMiscCrashInfo();

  /* Let client add application-specific files to report via crash callback. */

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

  CString sReportFolderName = m_sUnsentCrashReportsFolder + _T("\\") + m_sCrashGUID;
  BOOL bCreateDir = CreateDirectory(sReportFolderName, NULL);
  if(!bCreateDir)
  {    
    ATLASSERT(bCreateDir);
    CString szCaption;
    szCaption.Format(_T("%s has stopped working"), Utility::getAppName());
    CString szMessage;
    // Try notify user about crash using message box.
    szMessage.Format(_T("The program has stopped working due to unexpected error, but CrashRpt wasn't able to save the error report.\nPlease report about this issue at http://code.google.com/p/crashrpt/issues/list"));
    MessageBox(NULL, szMessage, szCaption, MB_OK|MB_ICONERROR);    
    return 1; // Failed to create directory
  }

  /* Create crash minidump file. */

  CString sFileName;
  sFileName.Format(_T("%s\\crashdump.dmp"), sReportFolderName);
  int result = CreateMinidump(sFileName, pExceptionInfo->pexcptrs);
  ATLASSERT(result==0);
  AddFile(sFileName, NULL, _T("Crash Dump"), 0);      
      
  /* Create crash report descriptor file in XML format. */
  
  sFileName.Format(_T("%s\\crashrpt.xml"), sReportFolderName);
  AddFile(sFileName, NULL, _T("Crash Log"), CR_AF_MISSING_FILE_OK);        
  result = GenerateCrashDescriptorXML(sFileName.GetBuffer(0), pExceptionInfo);
  ATLASSERT(result==0);
  
  
  // Launch the CrashSender process that would copy user-specified files to the error report folder, 
  // notify user about crash, compress the report into ZIP archive and send the error report by E-mail or HTTP.
    
  result = LaunchCrashSender(sReportFolderName);
  if(result!=0)
  {
    ATLASSERT(result==0);
    crSetErrorMsg(_T("Error launching CrashSender.exe"));
    
    // Failed to launch crash sender process.
    // Try notify user about crash using message box.
    CString szCaption;
    szCaption.Format(_T("%s has stopped working"), Utility::getAppName());
    CString szMessage;
    szMessage.Format(_T("The program has stopped working due to unexpected error, but CrashRpt wasn't able to run CrashSender.exe and send the error report.\nPlease report about this issue at http://code.google.com/p/crashrpt/issues/list"));
    MessageBox(NULL, szMessage, szCaption, MB_OK|MB_ICONERROR);    
    return 3;
  }

  // Notify g_CrashHandlers about crash, to avoid assertion in destructor
  g_CrashHandlers.m_bCrashHappened = TRUE;

  crSetErrorMsg(_T("Success."));
  return 0; 
}

void CCrashHandler::CollectMiscCrashInfo()
{  
  // Get crash time
  Utility::GetSystemTimeUTC(m_sCrashTime);

  // Get number of GUI resources in use
  HANDLE hCurProcess = GetCurrentProcess();
  m_dwGuiResources = GetGuiResources(hCurProcess, GR_GDIOBJECTS);
  
  // Get count of opened handles
  DWORD dwHandleCount = 0;
  BOOL bGetHandleCount = GetProcessHandleCount(hCurProcess, &dwHandleCount);
  if(bGetHandleCount)
    m_dwProcessHandleCount = dwHandleCount;
  else
    m_dwProcessHandleCount = 0;

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
  f = fopen(pszFileName, _T("wt"));
#endif
  
  if(f==NULL)
    return 1; // Couldn't create file

  // Add <?xml version="1.0" encoding="utf-8" ?> element
  fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n", CRASHRPT_VER);

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

    fprintf(f, "    <Prop name=\"%s\" value= \"%s\"/>\n",
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

int CCrashHandler::CreateMinidump(LPCTSTR pszFileName, EXCEPTION_POINTERS* pExInfo)
{   
  crSetErrorMsg(_T("Success."));

  // Create the file
  HANDLE hFile = CreateFile(
    pszFileName,
    GENERIC_WRITE,
    0,
    NULL,
    CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL,
    NULL);

  if(hFile==INVALID_HANDLE_VALUE)
  {
    ATLASSERT(hFile!=INVALID_HANDLE_VALUE);
    crSetErrorMsg(_T("Couldn't create file."));
    return 1;
  }

  // Write the minidump to the file
  
  MINIDUMP_EXCEPTION_INFORMATION eInfo;
  eInfo.ThreadId = GetCurrentThreadId();
  eInfo.ExceptionPointers = pExInfo;
  eInfo.ClientPointers = FALSE;

  MINIDUMP_CALLBACK_INFORMATION cbMiniDump;
  cbMiniDump.CallbackRoutine = NULL;
  cbMiniDump.CallbackParam = 0;

  BOOL bWriteDump = MiniDumpWriteDump(
    GetCurrentProcess(),
    GetCurrentProcessId(),
    hFile,
    MiniDumpNormal,
    pExInfo ? &eInfo : NULL,
    NULL,
    &cbMiniDump);
 
  if(!bWriteDump)
  {
    ATLASSERT(bWriteDump);
    crSetErrorMsg(_T("Couldn't write minidump to file."));
    return 2;
  }

  // Close file
  CloseHandle(hFile);

  return 0;
}

int CCrashHandler::LaunchCrashSender(CString sErrorReportDirName)
{
  crSetErrorMsg(_T("Success."));

  strconv_t strconv;

  /* Create CrashSender process */

  STARTUPINFO si;
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));  

  BOOL bCreateProcess = CreateProcess(m_sPathToCrashSender, 
    NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
  if(!bCreateProcess)
  {
    ATLASSERT(bCreateProcess);
    crSetErrorMsg(_T("Error creating CrashSender process."));
    return 1;
  }

  /* Connect to the pipe that CrashSender creates. */

  CString sPipeName;
  sPipeName.Format(_T("\\\\.\\pipe\\CrashRpt_%lu"), pi.dwProcessId);

  HANDLE hPipe = INVALID_HANDLE_VALUE;

  int MAX_ATTEMPTS = 30;
  int i;
  for(i=0; i<MAX_ATTEMPTS; i++)
  {
    hPipe = CreateFile(sPipeName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if(hPipe!=INVALID_HANDLE_VALUE)
      break;
    Sleep(1000);
  }

  if(hPipe==INVALID_HANDLE_VALUE)
  {
    ATLASSERT(hPipe!=INVALID_HANDLE_VALUE);
    crSetErrorMsg(_T("Error connecting to pipe."));
    return 2;
  }

  /* Transfer private crash information in XML format */

  CString sCrashInfo;
  
  sCrashInfo.Format(
    _T("<crashrpt subject=\"%s\" mailto=\"%s\" url=\"%s\" appname=\"%s\" \
appver=\"%s\" imagename=\"%s\" errorreportdirname=\"%s\" http_priority=\"%d\" \
smtp_priority=\"%d\" mapi_priority=\"%d\" privacy_policy_url=\"%s\">"), 
    _repxrch(m_sSubject), 
    _repxrch(m_sTo),
    _repxrch(m_sUrl),
    _repxrch(m_sAppName),
    _repxrch(m_sAppVersion),
    _repxrch(m_sImageName),
    _repxrch(sErrorReportDirName),
    m_uPriorities[CR_HTTP],
    m_uPriorities[CR_SMTP],
    m_uPriorities[CR_SMAPI],
    _repxrch(m_sPrivacyPolicyURL));

  CString sFileItems;
  sFileItems += _T("<FileItems>");
  std::map<CString, FileItem>::iterator it;
  for(it=m_files.begin(); it!=m_files.end(); it++)
  {
    CString sFileItem;
    sFileItem.Format(_T("<FileItem destfile=\"%s\" srcfile=\"%s\" description=\"%s\" makecopy=\"%d\" />"),
      _repxrch(it->first), 
      _repxrch(it->second.m_sFileName), 
      _repxrch(it->second.m_sDescription), 
      it->second.m_bMakeCopy?1:0);
    sFileItems += sFileItem;
  }
  sFileItems += _T("</FileItems>");

  sCrashInfo += sFileItems;
  sCrashInfo += _T("</crashrpt>");

  // Convert to multi-byte
  LPCSTR lpszCrashInfo =  strconv.t2a(sCrashInfo.GetBuffer(0));
  
  DWORD dwBytesWritten = 0;
  DWORD dwLength = (DWORD)strlen(lpszCrashInfo);
  BOOL bWrite = WriteFile(hPipe, lpszCrashInfo, dwLength, &dwBytesWritten, NULL);
  
  if(bWrite==FALSE || (int)dwBytesWritten == dwLength)
  {
    ATLASSERT(bWrite);
    ATLASSERT((int)dwBytesWritten == strlen(lpszCrashInfo));
    crSetErrorMsg(_T("Error transferring the crash information through the pipe."));
  }

  /* Clean up */

  CloseHandle(hPipe);

  return 0;
}

CString CCrashHandler::_repxrch(CString sText)
{
  CString sResult;

  sResult = sText;
  sResult.Replace(_T("\""), _T("&quot"));
  sResult.Replace(_T("'"), _T("&apos"));

  return sResult;
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



