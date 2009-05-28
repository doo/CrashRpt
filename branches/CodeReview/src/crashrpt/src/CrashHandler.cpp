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
#include "process.h"
#include "Utility.h"
#include "resource.h"


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

int __cdecl cpp_new_handler(size_t size)
{
   // 'new' operator memory allocation exception

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

CCrashHandler::CCrashHandler()
{
}

CCrashHandler::~CCrashHandler()
{
  Destroy();
}


int CCrashHandler::Init(
  LPCTSTR lpcszAppName,
  LPCTSTR lpcszAppVersion,
  LPGETLOGFILE lpfnCallback, 
  LPCTSTR lpcszTo, 
  LPCTSTR lpcszSubject)
{ 
  m_sAppName = lpcszAppName;

  if(m_sAppName.IsEmpty())
    m_sAppName = CUtility::getAppName();

  m_sAppVersion = lpcszAppVersion;
    
  // Get handle to the EXE module used to create this process
  HMODULE hModule = GetModuleHandle(NULL);
  if(hModule==NULL)
  {
    ATLASSERT(hModule!=NULL);
    return 1;
  }

  TCHAR szExeName[_MAX_PATH];
  DWORD dwLength = GetModuleFileName(hModule, szExeName, _MAX_PATH);
  if(GetLastError()!=0)
  {
    // Couldn't get the name of EXE that was used to create current process
    ATLASSERT(0);
    return 2;
  }

  m_sImageName = CString(szExeName, dwLength);
  
  // By default assume that CrashSender.exe is located in the same dir as EXE 
  m_sPathToCrashSender = CUtility::GetModulePath(hModule);   

  // initialize member data
  m_lpfnCallback = NULL;
  m_oldFilter    = NULL;

  // save user supplied callback
  if (lpfnCallback)
    m_lpfnCallback = lpfnCallback;

  // add this filter in the exception callback chain
  m_oldFilter = SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);

  // Set C++ exception handlers
  InitPrevCPPExceptionHandlerPointers();
   
  int nSetProcessHandlers = SetProcessCPPExceptionHandlers();   
  if(nSetProcessHandlers!=0)
  {
    ATLASSERT(nSetProcessHandlers==0);
    return 3;
  }

  int nSetThreadHandlers = SetThreadCPPExceptionHandlers();
  if(nSetThreadHandlers!=0)
  {
    ATLASSERT(nSetThreadHandlers==0);
    return 3;
  }

  // attach this handler with this process
  m_pid = _getpid();
  _crashStateMap.Add(m_pid, this);
   
  // save optional email info
  m_sTo = lpcszTo;
  m_sSubject = lpcszSubject;

  // OK.
  return 0;
}

int CCrashHandler::Destroy()
{
  // Reset exception callback
  if (m_oldFilter)
    SetUnhandledExceptionFilter(m_oldFilter);

  // All installed per-thread C++ exception handlers should be uninstalled 
  // using crUninstallFromCurrentThread() before calling Destroy()
  if(m_ThreadExceptionHandlers.size()!=0)
  {
    ATLASSERT(m_ThreadExceptionHandlers.size()==0);
  }

  _crashStateMap.Remove(m_pid);

  // OK.
  return 0;
}


// Sets internal pointers to previously used exception handlers to NULL
void CCrashHandler::InitPrevCPPExceptionHandlerPointers()
{
  m_prevPurec = NULL;
  m_prevInvpar = NULL;
  m_prevNewHandler = NULL;

#if _MSC_VER<1400    
   m_prevSec = NULL;
#endif

  m_prevSigABRT = NULL;
  m_prevSigFPE = NULL;
  m_prevSigINT = NULL;  
  m_prevSigTERM = NULL;
  
}

int CCrashHandler::SetProcessCPPExceptionHandlers()
{
  // Set CRT error mode
  // Write exception info to file
  HANDLE hLogFile = NULL;
  hLogFile = CreateFile(_T("crterror.log"), GENERIC_WRITE, FILE_SHARE_WRITE, 
      NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hLogFile==NULL)
  {
    ATLASSERT(hLogFile!=NULL);
    return 1;
  }

  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ERROR, hLogFile);


  // Catch pure virtual function calls.
  // Because there is one _purecall_handler for the whole process, 
  // calling this function immediately impacts all threads. The last 
  // caller on any thread sets the handler. 
  // http://msdn.microsoft.com/en-us/library/t296ys27.aspx
  m_prevPurec = _set_purecall_handler(cpp_purecall_handler);    

  // Catch invalid parameter exceptions.
  m_prevInvpar = _set_invalid_parameter_handler(cpp_invalid_parameter_handler); 

  // Catch new operator memory allocation exceptions
  m_prevNewHandler = _set_new_handler(cpp_new_handler);

  // Catch buffer overrun exceptions
  // The _set_security_error_handler is deprecated in VC8 C++ run time library
#if _MSC_VER<1400    
   m_prevSec = _set_security_error_handler(cpp_security_handler);
#endif

   // Set up C++ signal handlers
  
   // Catch an abnormal program termination
   m_prevSigABRT = signal(SIGABRT, cpp_sigabrt_handler);  

   // Catch a floating point error
   m_prevSigFPE = signal(SIGFPE, cpp_sigfpe_handler);     

   // Catch illegal instruction handler
   m_prevSigINT = signal(SIGINT, cpp_sigint_handler);     
   
   // Catch a termination request
   m_prevSigTERM = signal(SIGTERM, cpp_sigterm_handler);      

   return 0;
}

int CCrashHandler::UnSetProcessCPPExceptionHandlers()
{
  // Unset all previously set handlers

  if(m_prevPurec!=NULL)
    _set_purecall_handler(m_prevPurec);

  if(m_prevInvpar!=NULL)
    _set_invalid_parameter_handler(m_prevInvpar);

  if(m_prevNewHandler!=NULL)
    _set_new_handler(m_prevNewHandler);

#if _MSC_VER<1400    
  if(m_prevSec!=NULL)
    _set_security_error_handler(m_prevSec);
#endif
     
  if(m_prevSigABRT!=NULL)
    signal(SIGABRT, m_prevSigABRT);  

  if(m_prevSigFPE!=NULL)
    signal(SIGFPE, m_prevSigFPE);     

  if(m_prevSigINT!=NULL)
    signal(SIGINT, m_prevSigINT);     

  if(m_prevSigTERM!=NULL)
   signal(SIGTERM, m_prevSigTERM);    

  return 0;
}

// Installs C++ exception handlers that function on per-thread basis
int CCrashHandler::SetThreadCPPExceptionHandlers()
{
  DWORD dwThreadId = GetCurrentThreadId();

  std::map<DWORD, _cpp_thread_exception_handlers>::iterator it = 
    m_ThreadExceptionHandlers.find(dwThreadId);

  if(it!=m_ThreadExceptionHandlers.end())
  {
    // handlers are already set for the thread
    ATLASSERT(0);
    return 1; // failed
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

  // Catch an interrupt (CTRL+C)
  handlers.m_prevSigILL = signal(SIGILL, cpp_sigill_handler);     

  // Catch illegal storage access handler
  handlers.m_prevSigSEGV = signal(SIGSEGV, cpp_sigsegv_handler);   


  // Insert to list of handlers
  std::pair<DWORD, _cpp_thread_exception_handlers> _pair(dwThreadId, handlers);
  m_ThreadExceptionHandlers.insert(_pair);

  // OK.
  return 0;
}

int CCrashHandler::UnSetThreadCPPExceptionHandlers()
{
  DWORD dwThreadId = GetCurrentThreadId();

  std::map<DWORD, _cpp_thread_exception_handlers>::iterator it = 
    m_ThreadExceptionHandlers.find(dwThreadId);

  if(it==m_ThreadExceptionHandlers.end())
  {
    // no such handlers?
    ATLASSERT(0);
    return 1;
  }
  
  _cpp_thread_exception_handlers* handlers = &(it->second);

  if(handlers->m_prevTerm!=NULL)
    set_terminate(handlers->m_prevTerm);

  if(handlers->m_prevUnexp!=NULL)
    set_unexpected(handlers->m_prevUnexp);

  if(handlers->m_prevSigILL!=NULL)
    signal(SIGINT, handlers->m_prevSigILL);     

  if(handlers->m_prevSigSEGV!=NULL)
    signal(SIGSEGV, handlers->m_prevSigSEGV); 

  // OK.
  return 0;
}


int CCrashHandler::AddFile(LPCTSTR pszFile, LPCTSTR pszDesc)
{
   // make sure the file exist
   HANDLE hFile = ::CreateFile(
                     pszFile,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     0);

   if (hFile==INVALID_HANDLE_VALUE)
   {
     ATLASSERT(hFile!=INVALID_HANDLE_VALUE);
     return 1;
   }

   // Add file to file list.
   m_files[pszFile] = pszDesc;

   // Close handle
   ::CloseHandle(hFile);   

   // OK.
   return 0;
}

int CCrashHandler::GenerateErrorReport(PEXCEPTION_POINTERS pExInfo)
{
   CExceptionReport  rpt(pExInfo);   
   CZLib             zlib;
//   CString           sTempFileName = CUtility::getTempFileName();
   unsigned int      i;

   // let client add application specific files to report
   if (m_lpfnCallback && !m_lpfnCallback(this))
      return 1;

   // add crash files to report
   m_files[CStringA(rpt.getCrashFile())] = CString((LPCTSTR)IDS_CRASH_DUMP);
   CString sXmlName = rpt.getCrashLog();
   m_files[CStringA(sXmlName)] = CString((LPCTSTR)IDS_CRASH_LOG);

   // add symbol files to report
   for (i = 0; i < (UINT)rpt.getNumSymbolFiles(); i++)
      m_files[CStringA(rpt.getSymbolFile(i))] = 
      CString((LPCTSTR)IDS_SYMBOL_FILE);
 
   BOOL bSend = LaunchCrashSender();
   ATLASSERT(bSend);

   if(!bSend)
   {
     CString szCaption;
     szCaption.Format(_T("%s has stopped working"), CUtility::getAppName());
     
     CString szMessage;
     szMessage.Format(_T("This program has stopped working due to unexpected error. We are sorry for inconvenience.\nDo you want to save error report? Press Yes to save. Press No to close application."));
     INT_PTR res = MessageBox(NULL, szMessage, szCaption, MB_YESNO|MB_ICONERROR);
     if(res==IDYES)
     {
       CString sTempFileName = CUtility::getTempFileName();

       // zip the report
       if (!zlib.Open(sTempFileName))
         return 1;
   
       // add report files to zip
       TStrStrMap::iterator cur = m_files.begin();
       for (i = 0; i < m_files.size(); i++, cur++)
       {
         zlib.AddFile((char*)(LPCSTR)(*cur).first);
       }

       zlib.Close();

       // Send report
       BOOL bSave = CopyFile(sTempFileName, CUtility::getSaveFileName(), TRUE);
       ATLASSERT(bSave==TRUE);
     }     
   }

   return 0;

   //// display main dialog
   //mainDlg.m_pUDFiles = &m_files;
   //if (IDOK != mainDlg.DoModal())
   //{
   //  return;
   //}

   // Write user email and problem description to XML
   //rpt.writeUserInfo(szXMLName, mainDlg.m_sEmail, mainDlg.m_sDescription);

   // zip the report
   /*if (!zlib.Open(sTempFileName))
      return;*/
   
   // add report files to zip
   //TStrStrMap::iterator cur = m_files.begin();
   //for (i = 0; i < m_files.size(); i++, cur++)
   //{
   //  zlib.AddFile((char*)(LPCSTR)(*cur).first);
   //}

   //zlib.Close();

   //// Send report

   //if (m_sTo.IsEmpty() || 
   //    !MailReport(rpt, sTempFileName, mainDlg.m_sEmail, mainDlg.m_sDescription))
   //{
   //  SaveReport(rpt, sTempFileName);   
   //}

   //DeleteFile(sTempFileName);
}

//BOOL CCrashHandler::SaveReport(CExceptionReport&, LPCTSTR lpcszFile)
//{
//   // let user more zipped report
//   return (CopyFile(lpcszFile, CUtility::getSaveFileName(), TRUE));
//}


BOOL CCrashHandler::LaunchCrashSender()
{
  // Create CrashSender process

  CString sCrashSenderName;
  sCrashSenderName.Format(_T("%s\\%s"), 
    m_sPathToCrashSender,
#ifdef _DEBUG
    _T("CrashSenderd.exe")
#else
    _T("CrashSender.exe")
#endif //_DEBUG
    );

  STARTUPINFO si;
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));
  BOOL bCreateProcess = CreateProcess(sCrashSenderName, 
    NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
  if(!bCreateProcess)
  {
    ATLASSERT(bCreateProcess);
    return FALSE;
  }

  CString sPipeName;
  sPipeName.Format(_T("\\\\.\\pipe\\CrashRpt_%lu"), pi.dwProcessId);

  HANDLE hPipe = INVALID_HANDLE_VALUE;

  int MAX_ATTEMPTS = 120;
  int i;
  for(i=0; i<MAX_ATTEMPTS; i++)
  {
    hPipe = CreateFile(sPipeName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if(hPipe!=INVALID_HANDLE_VALUE)
      break;
    Sleep(1000);
  }

  if(hPipe==INVALID_HANDLE_VALUE)
    return FALSE;

  // Transfer crash files list
  CStringW sCrashInfo;
  sCrashInfo.Format(
    _T("<crashrpt subject=\"%s\" mailto=\"%s\" appname=\"%s\" imagename=\"%s\">"), 
    _ReplaceRestrictedXMLCharacters(m_sSubject), 
    _ReplaceRestrictedXMLCharacters(m_sTo),
    _ReplaceRestrictedXMLCharacters(m_sAppName),
    _ReplaceRestrictedXMLCharacters(m_sImageName));

  std::map<CStringA, CStringA>::iterator it;
  for(it=m_files.begin(); it!=m_files.end(); it++)
  {
    CString sName = _ReplaceRestrictedXMLCharacters(CString(it->first));
    CString sDesc = _ReplaceRestrictedXMLCharacters(CString(it->second));   
    
    CString sFile;
    sFile.Format(_T("<file name=\"%s\" description=\"%s\"/>"), sName, sDesc);
    sCrashInfo += sFile;
  }

  sCrashInfo += _T("</crashrpt>");

  DWORD dwBytesWritten = 0;
  WriteFile(hPipe, sCrashInfo.GetBuffer(), sCrashInfo.GetLength()*2, &dwBytesWritten, NULL);
  ATLASSERT((int)dwBytesWritten == sCrashInfo.GetLength()*2);

  // Clean up
  CloseHandle(hPipe);

  return TRUE;
}

CString CCrashHandler::_ReplaceRestrictedXMLCharacters(CString sText)
{
  CString sResult;

  sResult = sText;
  sResult.Replace(_T("\""), _T("&quot"));
  sResult.Replace(_T("'"), _T("&apos"));

  return sResult;
}

