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
#include <sys/stat.h>
#include "tinyxml.h"
#include <time.h>

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
  LPCTSTR lpcszCrashSenderPath,
  LPGETLOGFILE lpfnCallback, 
  LPCTSTR lpcszTo, 
  LPCTSTR lpcszSubject)
{ 
  crSetErrorMsg(_T("Unspecified error."));

  m_sAppName = lpcszAppName;

  if(m_sAppName.IsEmpty())
    m_sAppName = CUtility::getAppName();

  m_sAppVersion = lpcszAppVersion;
    
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
  if(GetLastError()!=0)
  {
    // Couldn't get the name of EXE that was used to create current process
    ATLASSERT(0);
    crSetErrorMsg(_T("Couldn't get the name of EXE that was used to create current process."));
    return 2;
  }  

  m_sImageName = CString(szExeName, dwLength);

  CString sCrashRptName;

#ifdef _DEBUG
  sCrashRptName = _T("CrashRptd.dll");
#else
  sCrashRptName = _T("CrashRpt.dll");
#endif //_DEBUG

  // Get handle to the CrashRpt module that is loaded by current process
  HMODULE hCrashRptModule = GetModuleHandle(sCrashRptName);
  if(hCrashRptModule==NULL)
  {
    ATLASSERT(hCrashRptModule!=NULL);
    crSetErrorMsg(_T("Couldn't get handle to CrashRpt.dll."));
    return 3;
  }  
  
  if(lpcszCrashSenderPath==NULL)
  {
    // By default assume that CrashSender.exe is located in the same dir as CrashRpt.dll
    m_sPathToCrashSender = CUtility::GetModulePath(hCrashRptModule);   
  }
  else
    m_sPathToCrashSender = CString(lpcszCrashSenderPath);    

  CString sCrashSenderName;

#ifdef _DEBUG
  sCrashSenderName = _T("CrashSenderd.exe");
#else
  sCrashSenderName = _T("CrashSender.exe");
#endif //_DEBUG

  if(m_sPathToCrashSender.Right(1)!='\\')
      m_sPathToCrashSender+="\\";
  m_sPathToCrashSender+=sCrashSenderName;   

  HANDLE hFile = CreateFile(m_sPathToCrashSender, FILE_GENERIC_READ, 
    FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  
  if(hFile==INVALID_HANDLE_VALUE)
  {
    ATLASSERT(hFile!=INVALID_HANDLE_VALUE);
    crSetErrorMsg(_T("Couldn't locate CrashSender.exe in specified directory."));
    return 3; // CrashSender not found!
  }

  CloseHandle(hFile);

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
    crSetErrorMsg(_T("Couldn't set C++ exception handlers for current process."));
    return 4;
  }

  int nSetThreadHandlers = SetThreadCPPExceptionHandlers();
  if(nSetThreadHandlers!=0)
  {
    ATLASSERT(nSetThreadHandlers==0);
    crSetErrorMsg(_T("Couldn't set C++ exception handlers for main execution thread."));
    return 5;
  }

  // attach this handler with this process
  m_pid = _getpid();
  _crashStateMap.Add(m_pid, this);
   
  // save optional email info
  m_sTo = lpcszTo;
  m_sSubject = lpcszSubject;

  // OK.
  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::Destroy()
{
  crSetErrorMsg(_T("Unspecified error."));

  // Reset exception callback
  if (m_oldFilter)
    SetUnhandledExceptionFilter(m_oldFilter);

  m_oldFilter = NULL;

  // All installed per-thread C++ exception handlers should be uninstalled 
  // using crUninstallFromCurrentThread() before calling Destroy()
  
  ATLASSERT(m_ThreadExceptionHandlers.size()==0);      
  m_ThreadExceptionHandlers.clear();

  _crashStateMap.Remove(m_pid);

  // OK.
  crSetErrorMsg(_T("Success."));
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
  crSetErrorMsg(_T("Unspecified error."));

  // Set CRT error mode
  // Write exception info to file
  HANDLE hLogFile = NULL;
  hLogFile = CreateFile(_T("crterror.log"), GENERIC_WRITE, FILE_SHARE_WRITE, 
      NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(hLogFile==NULL)
  {
    ATLASSERT(hLogFile!=NULL);
    crSetErrorMsg(_T("Couldn't create CRT log file."));
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

   crSetErrorMsg(_T("Success."));
   return 0;
}

int CCrashHandler::UnSetProcessCPPExceptionHandlers()
{
  crSetErrorMsg(_T("Unspecified error."));

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

  crSetErrorMsg(_T("Success."));
  return 0;
}

// Installs C++ exception handlers that function on per-thread basis
int CCrashHandler::SetThreadCPPExceptionHandlers()
{
  crSetErrorMsg(_T("Unspecified error."));

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

  // Catch terminate() calls. 
  // In a multithreaded environment, terminate functions are maintained 
  // separately for each thread. Each new thread needs to install its own 
  // terminate function. Thus, each thread is in charge of its own termination handling.
  // http://msdn.microsoft.com/en-us/library/t6fk7h29.aspx
  handlers.m_prevTerm = set_terminate(cpp_terminate_handler);       

  // Catch unexpected() calls.
  // In a multithreaded environment, unexpected functions are maintained 
  // separately for each thread. Each new thread needs to install its own 
  // unexpected function. Thus, each thread is in charge of its own unexpected handling.
  // http://msdn.microsoft.com/en-us/library/h46t5b69.aspx  
  handlers.m_prevUnexp = set_unexpected(cpp_unexp_handler);    

  // Catch an illegal instruction
  handlers.m_prevSigILL = signal(SIGILL, cpp_sigill_handler);     

  // Catch illegal storage access errors
  handlers.m_prevSigSEGV = signal(SIGSEGV, cpp_sigsegv_handler);   

  // Insert the structure to the list of handlers
  std::pair<DWORD, _cpp_thread_exception_handlers> _pair(dwThreadId, handlers);
  m_ThreadExceptionHandlers.insert(_pair);

  // OK.
  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::UnSetThreadCPPExceptionHandlers()
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


int CCrashHandler::AddFile(LPCTSTR pszFile, LPCTSTR pszDesc)
{
  crSetErrorMsg(_T("Unspecified error."));

  // make sure the file exist
  struct _stat st;
  int result = _tstat(pszFile, &st);

  if (result!=0)
  {
   ATLASSERT(0);
   crSetErrorMsg(_T("Couldn't stat file. File may not exist."));
   return 1;
  }

  // Add file to file list.
  m_files[pszFile] = pszDesc;

  // OK.
  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::GenerateErrorReport(
  PEXCEPTION_POINTERS pExInfo, PCR_EXCEPTION_INFO pAdditionalInfo)
{  
  crSetErrorMsg(_T("Unspecified error."));

  /* Let client add application-specific files to report via crash callback. */

  if (m_lpfnCallback!=NULL && m_lpfnCallback(NULL)==FALSE)
  {
    crSetErrorMsg(_T("The operation was cancelled by client application."));
    return 1;
  }

  /* Create crash minidump and crash log. */

  CString sTempDir = CUtility::getTempFileName();
  DeleteFile(sTempDir);

  BOOL bCreateDir = CreateDirectory(sTempDir, NULL);  
  if(bCreateDir)
  {
    /* Create crash minidump file */

    CString sFileName;
    sFileName.Format(_T("%s\\crashdump.dmp"), sTempDir);
    int result = CreateMinidump(sFileName.GetBuffer(), pExInfo);
    ATLASSERT(result==0);

    if(result==0)
    {
      m_files[CStringA(sFileName)] = CString((LPCTSTR)IDS_CRASH_DUMP);
    }

    /* Create crash log file in XML format. */
  
    sFileName.Format(_T("%s\\crashlog.xml"), sTempDir, CUtility::getAppName());
    result = GenerateCrashLogXML(sFileName, pExInfo, pAdditionalInfo);
    ATLASSERT(result==0);

    if(result==0)
    {
      m_files[CStringA(sFileName)] = CString((LPCTSTR)IDS_CRASH_LOG);
    }
  }
     
  // Launch the CrashSender process that would notify user about crash
  // and send the error report by E-mail.
  
  int result = LaunchCrashSender();
  if(result!=0)
  {
    ATLASSERT(result==0);
    // Failed to launch crash sender process.
    // Try notify user about crash using message box.
    EmergencyNotifyUser();      
    crSetErrorMsg(_T("Error sending error report by E-mail."));
    return 2;
  }

  crSetErrorMsg(_T("Success."));
  return 0; 
}

int CCrashHandler::GenerateCrashLogXML(PCTSTR pszFileName, PEXCEPTION_POINTERS pExInfo,         
     PCR_EXCEPTION_INFO pAdditionalInfo)
{
  crSetErrorMsg(_T("Unspecified error."));

  TiXmlDocument doc;
  
  TiXmlElement* root = new TiXmlElement("CrashRpt");
  doc.LinkEndChild(root);

  CStringA sCrashRptVer;
  sCrashRptVer.Format("%d", CRASHRPT_VER);
  root->SetAttribute("version", sCrashRptVer);


  // Write application name 

  TiXmlElement* app_name = new TiXmlElement("AppName");
  root->LinkEndChild(app_name);

  TiXmlText* app_name_text = new TiXmlText(CStringA(m_sAppName));
  app_name->LinkEndChild(app_name_text);

  // Write application version 

  TiXmlElement* app_ver = new TiXmlElement("AppVersion");
  root->LinkEndChild(app_ver);

  TiXmlText* app_ver_text = new TiXmlText(CStringA(m_sAppVersion));
  app_ver->LinkEndChild(app_ver_text);

  // Write EXE image name

  TiXmlElement* image_name = new TiXmlElement("ImageName");
  root->LinkEndChild(image_name);

  TiXmlText* image_name_text = new TiXmlText(CStringA(m_sImageName));
  image_name->LinkEndChild(image_name_text);

  // Write operating system friendly name

  CRegKey regKey;
  LONG lResult = regKey.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), KEY_READ);
  if(lResult==ERROR_SUCCESS)
  {
    CStringA sOSName;
    
    TCHAR buf[1024];
    ULONG buf_size = 0;

    buf_size = 1024;
    if(ERROR_SUCCESS == regKey.QueryStringValue(_T("ProductName"), buf, &buf_size))
      sOSName = CString(buf, buf_size);
    
    buf_size = 1024;
    if(ERROR_SUCCESS == regKey.QueryStringValue(_T("CurrentBuildNumber"), buf, &buf_size))
      sOSName += _T(" Build ") + CString(buf, buf_size);

    buf_size = 1024;
    if(ERROR_SUCCESS == regKey.QueryStringValue(_T("CSDVersion"), buf, &buf_size))
      sOSName += _T(" ") + CString(buf, buf_size);

    TiXmlElement* os_name = new TiXmlElement("OperatingSystem");
    root->LinkEndChild(os_name);

    TiXmlText* os_name_text = new TiXmlText(sOSName.GetBuffer());
    os_name->LinkEndChild(os_name_text);

    regKey.Close();
  }

  // Write system time in UTC format

  time_t cur_time;
  time(&cur_time);
  char szDateTime[64];

  struct tm timeinfo;
  gmtime_s(&timeinfo, &cur_time);
  strftime(szDateTime, 64,  "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

  TiXmlElement* sys_time = new TiXmlElement("SystemTimeUTC");
  root->LinkEndChild(sys_time);

  TiXmlText* sys_time_text = new TiXmlText(szDateTime);
  sys_time->LinkEndChild(sys_time_text);


  // Save document to file

  bool bSave = doc.SaveFile(CStringA(pszFileName).GetBuffer());
  if(!bSave)
  {
    ATLASSERT(bSave);
    crSetErrorMsg(_T("Can't save crash log to XML."));
    return 2;
  }

  crSetErrorMsg(_T("Success."));
  return 0;
}

int CCrashHandler::CreateMinidump(PCTSTR pszFileName, EXCEPTION_POINTERS* pExInfo)
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

int CCrashHandler::LaunchCrashSender()
{
  crSetErrorMsg(_T("Success."));

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
    crSetErrorMsg(_T("Error crating CrashSender process."));
    return 1;
  }

  /* Connect to the pipe that CrashSender creates. */

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
  {
    ATLASSERT(hPipe!=INVALID_HANDLE_VALUE);
    crSetErrorMsg(_T("Error connecting to pipe."));
    return 2;
  }

  /* Transfer crash information in XML format */

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
  BOOL bWrite = WriteFile(hPipe, sCrashInfo.GetBuffer(), sCrashInfo.GetLength()*2, &dwBytesWritten, NULL);
  
  if(!bWrite || (int)dwBytesWritten == sCrashInfo.GetLength()*2)
  {
    ATLASSERT(bWrite);
    ATLASSERT((int)dwBytesWritten == sCrashInfo.GetLength()*2);
    crSetErrorMsg(_T("Error transferring the crash information through the pipe."));
  }

  /* Clean up */

  CloseHandle(hPipe);

  return 0;
}

int CCrashHandler::EmergencyNotifyUser()
{
  crSetErrorMsg(_T("Success."));

  CString szCaption;
  szCaption.Format(_T("%s has stopped working"), CUtility::getAppName());
   
  CString szMessage;
  szMessage.Format(_T("This program has stopped working due to unexpected error. We are sorry for inconvenience.\nPress Yes to save the error report. Press No to close application."));
  INT_PTR res = MessageBox(NULL, szMessage, szCaption, MB_YESNO|MB_ICONERROR);
  if(res==IDYES)
  {
    CString sTempFileName = CUtility::getTempFileName();

    CZLib zlib;
    
    // zip the report
    if (!zlib.Open(sTempFileName))
    {
      crSetErrorMsg(_T("Couldn't zip the error report."));
      return 1;
    }

    // add report files to zip
    TStrStrMap::iterator cur = m_files.begin();
    unsigned i;
    for (i = 0; i < m_files.size(); i++, cur++)
    {
      zlib.AddFile((char*)(LPCSTR)(*cur).first);
    }

    zlib.Close();

    // Send report
    BOOL bSave = CopyFile(sTempFileName, CUtility::getSaveFileName(), TRUE);
    if(bSave==FALSE)
    {
      ATLASSERT(bSave==TRUE);
      crSetErrorMsg(_T("Couldn't save error report to specified file."));
      return 1;
    }
  }

  return 0;
}

CString CCrashHandler::_ReplaceRestrictedXMLCharacters(CString sText)
{
  CString sResult;

  sResult = sText;
  sResult.Replace(_T("\""), _T("&quot"));
  sResult.Replace(_T("'"), _T("&apos"));

  return sResult;
}

CCrashHandler* CCrashHandler::GetCurrentProcessCrashHandler()
{
  int pid = _getpid();
  return _crashStateMap.Lookup(pid);
}



