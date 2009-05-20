#include "stdafx.h"
#include "CrashThread.h"
#include <exception>
#include <signal.h>
#include <assert.h>

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

int GenerateException(eExceptionType ExceptionType)
{
  switch(ExceptionType)
  {
  case ET_NO_EXCEPTION:
    break; // no exception
  case ET_UNHANDLED_WIN32_EXCEPTION:
    {
      int *p = 0;
      *p = 0;
    }
    break;
  case ET_CPP_TERMINATE:
    {
      terminate();
    }
    break;
  case ET_CPP_UNEXPECTED:
    {
      unexpected();
    }
    break;
  case ET_CPP_PURECALL:
    {
      // pure virtual method call
      CDerived derived;
    }
    break;
  case ET_CPP_SECURITY:
    {
      // Cause buffer overrun (/GS compiler option)

      char large_buffer[] = "This string is longer than 10 characters!!!";
      // vulnerable code
      char buffer[10];
#pragma warning(suppress:4996) // avoid C4996 warning
      strcpy(buffer, large_buffer); // overrun buffer !!!      
    }
    break;
  case ET_CPP_INVALIDPARAM:
    {
      char* formatString;
      // Call printf_s with invalid parameters.
      formatString = NULL;
      printf(formatString);
    }
    break;
  case ET_CPP_NEW:
    {
      RecurseAlloc();
    }
    break;
  case ET_CPP_SIGABRT: 
    {
      abort();
    }
    break;
  case ET_CPP_SIGFPE:
    {
      // floating point exception ( /fp:except compiler option)
      sigfpe_test(1.0f);
    }
    break;
  case ET_CPP_SIGILL: 
    {
      int result = raise(SIGILL);  
      ATLASSERT(result==0);
      return result;
    }
    break;
  case ET_CPP_SIGINT: 
    {
      int result = raise(SIGINT);  
      ATLASSERT(result==0);
      return result;
    }
    break;
  case ET_CPP_SIGSEGV: 
    {
      int result = raise(SIGSEGV);  
      ATLASSERT(result==0);
      return result;
    }
    break;
  case ET_CPP_SIGTERM: 
    {
     int result = raise(SIGTERM);  break;
     ATLASSERT(result==0);
     return result;
    }
  default:
    assert(0); // unknown type?
    return 1;
  }

  return 0;
}


DWORD WINAPI CrashThread(LPVOID pParam)
{
  CrashThreadInfo* pInfo = (CrashThreadInfo*)pParam;

  // Install per-thread C++ exception handlers
  crInstallToCurrentThread(pInfo->m_pCrashRptState);

  for(;;)
  {
    // Wait until wake up event is signaled
    WaitForSingleObject(pInfo->m_hWakeUpEvent, INFINITE);   

    if(GenerateException(pInfo->m_ExceptionType)!=0)
    {
      crUninstallFromCurrentThread(pInfo->m_pCrashRptState);
      ResetEvent(pInfo->m_hWakeUpEvent);
      return 1;  
    }
  }

  // Uninstall handlers from current thread
  crUninstallFromCurrentThread(pInfo->m_pCrashRptState);
  ResetEvent(pInfo->m_hWakeUpEvent);

  // Exit this thread
  return 0;
}




