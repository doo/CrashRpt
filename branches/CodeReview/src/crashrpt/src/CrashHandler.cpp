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
#include <atlstr.h>
#include "CrashHandler.h"
#include "zlibcpp.h"
#include "excprpt.h"
#include "maindlg.h"
#include "process.h"
#include "mailmsg.h"
#include <assert.h>


// global app module
CAppModule _Module;

// maps crash objects to processes
CSimpleMap<int, CCrashHandler*> _crashStateMap;

// unhandled exception callback set with SetUnhandledExceptionFilter()
LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExInfo)
{
   _crashStateMap.Lookup(_getpid())->GenerateErrorReport(pExInfo);

   return EXCEPTION_EXECUTE_HANDLER;
}


void __cdecl cpp_terminate_handler()
{
  // Abnormal program termination (terminate() function was called)

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1); 
}

void __cdecl cpp_unexp_handler()
{
  // Unexpected error (unexpected() function was called)

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1); 
}


void __cdecl cpp_purecall_handler()
{
  // Pure virtual function call
  
  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1); 
}

void __cdecl cpp_security_handler(int code, void *x)
{
  // Security error (buffer overrun).

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report  
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  exit(1); // Terminate program 
}

void __cdecl cpp_invalid_parameter_handler(const wchar_t* expression, const wchar_t* function, 
  const wchar_t* file, unsigned int line, uintptr_t pReserved)
 {
   // Invalid parameter exception

   // Generate crash report (although without EXCEPTION_POINTERS information)
   // TODO: need to write additional info about this exception to crash report 
   _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

   exit(1); // Terminate program
 }

void cpp_sigabrt_handler(int sig)
{
  // Caught SIGABRT C++ signal

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1);
}

void cpp_sigfpe_handler(int sig)
{
  // Floating point exception (SIGFPE)

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1);
}

void cpp_sigill_handler(int sig)
{
  // Illegal instruction (SIGILL)

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1);
}

void cpp_sigint_handler(int sig)
{
  // Interruption (SIGINT)

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1);
}

void cpp_sigsegv_handler(int sig)
{
  // Invalid storage access (SIGSEGV)

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1);
}

void cpp_sigterm_handler(int sig)
{
  // Termination request (SIGTERM)

  // Generate crash report (although without EXCEPTION_POINTERS information)
  // TODO: need to write additional info about this exception to crash report
  _crashStateMap.Lookup(_getpid())->GenerateErrorReport(0);

  // Terminate program
  exit(1);
}

CCrashHandler::CCrashHandler(LPGETLOGFILE lpfn /*=NULL*/, 
                             LPCTSTR lpcszTo /*=NULL*/, 
                             LPCTSTR lpcszSubject /*=NULL*/)
{
   // wtl initialization stuff...
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

   hRes = _Module.Init(NULL, GetModuleHandle("CrashRpt.dll"));
   ATLASSERT(SUCCEEDED(hRes));

	::DefWindowProc(NULL, 0, 0, 0L);

   // initialize member data
   m_lpfnCallback = NULL;
   m_oldFilter    = NULL;

   // save user supplied callback
   if (lpfn)
      m_lpfnCallback = lpfn;

   // add this filter in the exception callback chain
   m_oldFilter = SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);

   // Set C++ exception handlers
   InitPrevCPPExceptionHandlerPointers();
   SetProcessCPPExceptionHandlers();   
   SetThreadCPPExceptionHandlers();

   // attach this handler with this process
   m_pid = _getpid();
   _crashStateMap.Add(m_pid, this);

   // save optional email info
   m_sTo = lpcszTo;
   m_sSubject = lpcszSubject;
}

CCrashHandler::~CCrashHandler()
{
   // reset exception callback
   if (m_oldFilter)
      SetUnhandledExceptionFilter(m_oldFilter);

   _crashStateMap.Remove(m_pid);

   // uninitialize
   _Module.Term();
	::CoUninitialize();

}

// Sets internal pointers to previously used exception handlers to NULL
void CCrashHandler::InitPrevCPPExceptionHandlerPointers()
{
  m_prevPurec = NULL;
  m_prevInvpar = NULL;

#if _MSC_VER<1400    
   m_prevSec = NULL;
#endif

  m_prevSigABRT = NULL;
  m_prevSigFPE = NULL;
  m_prevSigILL = NULL;
  m_prevSigINT = NULL;
  m_prevSigSEGV = NULL;
  m_prevSigTERM = NULL;
  
}

void CCrashHandler::SetProcessCPPExceptionHandlers()
{
  // Catch pure virtual function calls.
  // Because there is one _purecall_handler for the whole process, 
  // calling this function immediately impacts all threads. The last 
  // caller on any thread sets the handler. 
  // http://msdn.microsoft.com/en-us/library/t296ys27.aspx
  m_prevPurec = _set_purecall_handler(cpp_purecall_handler);    

  // Catch invalid parameter exceptions.
  m_prevInvpar = _set_invalid_parameter_handler(cpp_invalid_parameter_handler); 

  // Catch buffer overrun exceptions
  // The _set_security_error_handler is deprecated in VC8 C++ run time library
#if _MSC_VER<1400    
   m_prevSec = _set_security_error_handler(cpp_security_handler);
#endif

   // Set up C++ signal handlers
   //typedef void (__cdecl *sigh)(int);    

   // Catch an abnormal program termination
   m_prevSigABRT = signal(SIGABRT, cpp_sigabrt_handler);  

   // Catch a floating point error
   m_prevSigFPE = signal(SIGFPE, cpp_sigfpe_handler);     

   // Catch illegal instruction handler
   m_prevSigILL = signal(SIGILL, cpp_sigill_handler);     

   // Catch an interrupt (CTRL+C)
   m_prevSigINT = signal(SIGINT, cpp_sigint_handler);     

   // Catch illegal storage access handler
   m_prevSigSEGV = signal(SIGSEGV, cpp_sigsegv_handler);   

   // Catch a termination request
   m_prevSigTERM = signal(SIGTERM, cpp_sigterm_handler);      
}

void CCrashHandler::UnSetProcessCPPExceptionHandlers()
{
  // Unset all previously set handlers

  if(m_prevPurec!=NULL)
    _set_purecall_handler(m_prevPurec);

  if(m_prevInvpar!=NULL)
    _set_invalid_parameter_handler(m_prevInvpar);

#if _MSC_VER<1400    
  if(m_prevSec!=NULL)
    _set_security_error_handler(m_prevSec);
#endif
     
  if(m_prevSigABRT!=NULL)
    signal(SIGABRT, m_prevSigABRT);  

  if(m_prevSigFPE!=NULL)
    signal(SIGFPE, m_prevSigFPE);     

  if(m_prevSigILL!=NULL)
    signal(SIGILL, m_prevSigILL);     

  if(m_prevSigINT!=NULL)
    signal(SIGINT, m_prevSigINT);     

  if(m_prevSigSEGV!=NULL)
    signal(SIGSEGV, m_prevSigSEGV);   

  if(m_prevSigTERM!=NULL)
   signal(SIGTERM, m_prevSigTERM);    
}

// Installs C++ exception handlers that function on per-thread basis
void CCrashHandler::SetThreadCPPExceptionHandlers()
{
  DWORD dwThreadId = GetCurrentThreadId();

  std::map<DWORD, _cpp_thread_exception_handlers>::iterator it = 
    m_ThreadExceptionHandlers.find(dwThreadId);

  if(it!=m_ThreadExceptionHandlers.end())
  {
    // handlers are already set for the thread
    assert(0);
    return; // failed
  }
  
  _cpp_thread_exception_handlers handlers;

  // Catch termination requests. 
  // In a multithreaded environment, terminate functions are maintained 
  // separately for each thread. Each new thread needs to install its own 
  // terminate function. Thus, each thread is in charge of its own termination handling.
  // http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
  handlers.m_prevTerm = set_terminate(cpp_terminate_handler);       

  // Catch unexpected errors.
  // In a multithreaded environment, unexpected functions are maintained 
  // separately for each thread. Each new thread needs to install its own 
  // unexpected function. Thus, each thread is in charge of its own unexpected handling.
  // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
  handlers.m_prevUnexp = set_unexpected(cpp_unexp_handler);    

  // Insert to list of handlers
  std::pair<DWORD, _cpp_thread_exception_handlers> _pair(dwThreadId, handlers);
  m_ThreadExceptionHandlers.insert(_pair);
}

void CCrashHandler::UnSetThreadCPPExceptionHandlers()
{
  DWORD dwThreadId = GetCurrentThreadId();

  std::map<DWORD, _cpp_thread_exception_handlers>::iterator it = 
    m_ThreadExceptionHandlers.find(dwThreadId);

  if(it==m_ThreadExceptionHandlers.end())
  {
    // no such handlers?
    assert(0);
    return;
  }
  
  _cpp_thread_exception_handlers* handlers = &(it->second);

  if(handlers->m_prevTerm!=NULL)
    set_terminate(handlers->m_prevTerm);

  if(handlers->m_prevUnexp!=NULL)
    set_unexpected(handlers->m_prevUnexp);
}


void CCrashHandler::AddFile(LPCTSTR lpFile, LPCTSTR lpDesc)
{
   // make sure the file exist
   HANDLE hFile = ::CreateFile(
                     lpFile,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     0);
   if (hFile)
   {
      // add file to report
      m_files[lpFile] = lpDesc;
      ::CloseHandle(hFile);
   }
}

void CCrashHandler::GenerateErrorReport(PEXCEPTION_POINTERS pExInfo)
{
   CExceptionReport  rpt(pExInfo);
   CMainDlg          mainDlg;
   CZLib             zlib;
   CString           sTempFileName = CUtility::getTempFileName();
   unsigned int      i;

   // let client add application specific files to report
   if (m_lpfnCallback && !m_lpfnCallback(this))
      return;

   // add crash files to report
   m_files[rpt.getCrashFile()] = CString((LPCTSTR)IDS_CRASH_DUMP);
   ATL::CString szXMLName = rpt.getCrashLog();
   m_files[szXMLName] = CString((LPCTSTR)IDS_CRASH_LOG);

   // add symbol files to report
   for (i = 0; i < (UINT)rpt.getNumSymbolFiles(); i++)
      m_files[(LPCTSTR)rpt.getSymbolFile(i)] = 
      CString((LPCTSTR)IDS_SYMBOL_FILE);
 
   // display main dialog
   mainDlg.m_pUDFiles = &m_files;
   if (IDOK != mainDlg.DoModal())
   {
     return;
   }

   // Write user email and problem description to XML
   rpt.writeUserInfo(szXMLName, mainDlg.m_sEmail, mainDlg.m_sDescription);

   // zip the report
   if (!zlib.Open(sTempFileName))
      return;
   
   // add report files to zip
   TStrStrMap::iterator cur = m_files.begin();
   for (i = 0; i < m_files.size(); i++, cur++)
      zlib.AddFile((*cur).first);

   zlib.Close();

   // Send report

   if (m_sTo.IsEmpty() || 
       !MailReport(rpt, sTempFileName, mainDlg.m_sEmail, mainDlg.m_sDescription))
   {
     SaveReport(rpt, sTempFileName);   
   }

   DeleteFile(sTempFileName);
}

BOOL CCrashHandler::SaveReport(CExceptionReport&, LPCTSTR lpcszFile)
{
   // let user more zipped report
   return (CopyFile(lpcszFile, CUtility::getSaveFileName(), TRUE));
}

BOOL CCrashHandler::MailReport(CExceptionReport&, LPCTSTR lpcszFile,
                               LPCTSTR lpcszEmail, LPCTSTR lpcszDesc)
{
   CMailMsg msg;
   msg
      .SetTo(m_sTo)
      .SetFrom(lpcszEmail)
      .SetSubject(m_sSubject.IsEmpty()?_T("Incident Report"):m_sSubject)
      .SetMessage(lpcszDesc)
      .AddAttachment(lpcszFile, CUtility::getAppName() + _T(".zip"));

   return (msg.Send());
}
