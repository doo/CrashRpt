/*! \file   CrashRpt.cpp
 *  \brief  Implementation of CrashRpt API
 *  \date   2003-2009
 *  \author Copyright (c) 2003 Michael Carruth
 *  \author zexspectrum_1980@mail.ru
 *  \todo
 */

#include "stdafx.h"
#include "CrashRpt.h"
#include "CrashHandler.h"


CRASHRPTAPI LPVOID Install(LPGETLOGFILE pfnCallback, LPCTSTR pszEmailTo, LPCTSTR pszEmailSubject)
{
  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = sizeof(CR_INSTALL_INFO);
  info.pfnCrashCallback = pfnCallback;
  info.pszEmailTo = pszEmailTo;
  info.pszEmailSubject = pszEmailSubject;

  crInstall(&info);

  return NULL;
}

CRASHRPTAPI void Uninstall(LPVOID lpState)
{
  crUninstall();  
}

CRASHRPTAPI void AddFile(LPVOID lpState, LPCTSTR lpFile, LPCTSTR lpDesc)
{ 
  crAddFile(lpFile, lpDesc);
}

CRASHRPTAPI void GenerateErrorReport(LPVOID lpState, PEXCEPTION_POINTERS pExInfo)
{
  crGenerateErrorReport(pExInfo, NULL);
}

CRASHRPTAPI int crInstall(CR_INSTALL_INFO* pInfo)
{
  // Validate input parameters.
  if(pInfo==NULL || 
     pInfo->cb!=sizeof(CR_INSTALL_INFO))
  {
    ATLASSERT(pInfo->cb==sizeof(CR_INSTALL_INFO));
    ATLASSERT(pInfo != NULL);    
    return 1; // Invalid parameter?
  }

  // Check if crInstall() already was called for current process.
  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler!=NULL)
  {
    ATLASSERT(pCrashHandler==NULL);
    return 2; // Installing to the same process twice?
  }
  
  pCrashHandler = new CCrashHandler();
  if(pCrashHandler==NULL)
  {
    ATLASSERT(pCrashHandler!=NULL);
    return 3; // Memory allocation error?
  }

  int nInitResult = pCrashHandler->Init(
    pInfo->pszAppName, 
    pInfo->pszAppVersion, 
    pInfo->pszCrashSenderPath,
    pInfo->pfnCrashCallback,
    pInfo->pszEmailTo,
    pInfo->pszEmailSubject);
  
  if(nInitResult!=0)
  {
    ATLASSERT(nInitResult==0);    
    return 4;// Crash handler initialization error?
  }

  // OK.
  return 0;
}

CRASHRPTAPI int crUninstall()
{
  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();
  
  if(pCrashHandler==NULL)
  {
    //nothing to uninstall
    ATLASSERT(pCrashHandler!=NULL);
    return 1; // The crInstall() wasn't previously called for this process    
  }

  // Uninstall main thread's C++ exception handlers
  pCrashHandler->UnSetThreadCPPExceptionHandlers();

  pCrashHandler->Destroy();

  delete pCrashHandler;

  // OK.
  return 0;
}

// Sets C++ exception handlers for the calling thread
CRASHRPTAPI int crInstallToCurrentThread()
{
  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();
  
  if(pCrashHandler==NULL)
  {
    ATLASSERT(pCrashHandler!=NULL);
    return 1; // Invalid parameter?
  }

  int nResult = pCrashHandler->SetThreadCPPExceptionHandlers();
  if(nResult!=0)
    return 2; // Error?

  // Ok.
  return 0;
}

// Unsets C++ exception handlers from the calling thread
CRASHRPTAPI int crUninstallFromCurrentThread()
{
  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {
    ATLASSERT(pCrashHandler!=NULL);
    return 1; // Invalid parameter?
  }

  int nResult = pCrashHandler->UnSetThreadCPPExceptionHandlers();
  if(nResult!=0)
    return 2; // Error?

  // OK.
  return 0;
}

CRASHRPTAPI int crAddFile(PCTSTR pszFile, PCTSTR pszDesc)
{
  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {
    ATLASSERT(pCrashHandler!=NULL);
    return 1; // No handler installed for current process?
  }
   
  int nAddResult = pCrashHandler->AddFile(pszFile, pszDesc);
  if(nAddResult==0)
  {
    ATLASSERT(nAddResult==0);
    return 2; // Couldn't add file?
  }

  // OK.
  return 0;
}

CRASHRPTAPI int crGenerateErrorReport(
  _EXCEPTION_POINTERS* pExInfo, CR_EXCEPTION_INFO* pAdditionalInfo)
{
  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {    
    // Handler is not installed for current process 
    ATLASSERT(pCrashHandler!=NULL);
    return 1;
  } 

  return pCrashHandler->GenerateErrorReport(pExInfo, pAdditionalInfo);  
}


CRASHRPTAPI int crExceptionFilter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
{
  CCrashHandler *pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();

  if(pCrashHandler==NULL)
  {    
    // Handler is not installed for current process 
    return EXCEPTION_CONTINUE_SEARCH; 
  }

  CR_EXCEPTION_INFO additional_info;
  additional_info.cb = sizeof(CR_EXCEPTION_INFO);
  additional_info.code = code;
  additional_info.subcode = 0;

  pCrashHandler->GenerateErrorReport(ep, &additional_info);
    
  return EXCEPTION_EXECUTE_HANDLER;
}

//-----------------------------------------------------------------------------------------------
// Below crEmulateCrash() related stuff goes 


class CDerived;
class CBase
{
public:
   CBase(CDerived *derived): m_pDerived(derived) {};
   ~CBase();
   virtual void function(void) = 0;

   CDerived * m_pDerived;
};

class CDerived : public CBase
{
public:
   CDerived() : CBase(this) {};   // C4355
   virtual void function(void) {};
};

CBase::~CBase()
{
   m_pDerived -> function();
}

#pragma warning(disable: 4723) // avoid C4723 warning
float sigfpe_test(float a)
{
  // division by zero
  return (a/0.0f);
}

#define BIG_NUMBER 0x1fffffff

int coalesced = 0;

#pragma warning(disable: 4717) // avoid C4717 warning
int RecurseAlloc() 
{
   int *pi = new int[BIG_NUMBER];
   pi;
   RecurseAlloc();
   return 0;
}

/*CRASHRPTAPI*/ int crEmulateCrash(unsigned ExceptionType)
{
  switch(ExceptionType)
  {
  case CR_WIN32_NULL_POINTER_EXCEPTION:
    {
      int *p = 0;
      *p = 0;
    }
    break;
  case CR_CPP_TERMINATE_CALL:
    {
      terminate();
    }
    break;
  case CR_CPP_UNEXPECTED_CALL:
    {
      unexpected();
    }
    break;
  case CR_CPP_PURE_CALL:
    {
      // pure virtual method call
      CDerived derived;
    }
    break;
  case CR_CPP_SECURITY_ERROR:
    {
      // Cause buffer overrun (/GS compiler option)

      char large_buffer[] = "This string is longer than 10 characters!!!";
      // vulnerable code
      char buffer[10];
#pragma warning(suppress:4996) // avoid C4996 warning
      strcpy(buffer, large_buffer); // overrun buffer !!!      
    }
    break;
  case CR_CPP_INVALID_PARAMETER:
    {
      char* formatString;
      // Call printf_s with invalid parameters.
      formatString = NULL;
      printf(formatString);
    }
    break;
  case CR_CPP_NEW_OPERATOR_ERROR:
    {
      RecurseAlloc();
    }
    break;
  case CR_CPP_SIGABRT: 
    {
      abort();
    }
    break;
  case CR_CPP_SIGFPE:
    {
      // floating point exception ( /fp:except compiler option)
      sigfpe_test(1.0f);
    }
    break;
  case CR_CPP_SIGILL: 
    {
      int result = raise(SIGILL);  
      ATLASSERT(result==0);
      return result;
    }
    break;
  case CR_CPP_SIGINT: 
    {
      int result = raise(SIGINT);  
      ATLASSERT(result==0);
      return result;
    }
    break;
  case CR_CPP_SIGSEGV: 
    {
      int result = raise(SIGSEGV);  
      ATLASSERT(result==0);
      return result;
    }
    break;
  case CR_CPP_SIGTERM: 
    {
     int result = raise(SIGTERM);  break;
     ATLASSERT(result==0);
     return result;
    }
  default:
    ATLASSERT(0); // unknown type?
    return 1;
  }
 
  return 0;
}



