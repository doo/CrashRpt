#include "stdafx.h"
#include "CrashThread.h"
#include <exception>
#include <signal.h>
#include <assert.h>

DWORD WINAPI CrashThread(LPVOID pParam)
{
  CrashThreadInfo* pInfo = (CrashThreadInfo*)pParam;

  // Install per-thread C++ exception handlers
  crInstallToCurrentThread();

  for(;;)
  {
    // Wait until wake up event is signaled
    WaitForSingleObject(pInfo->m_hWakeUpEvent, INFINITE);   

    if(crEmulateCrash(pInfo->m_ExceptionType)!=0)
    {
      crUninstallFromCurrentThread();
      ResetEvent(pInfo->m_hWakeUpEvent);
      break;
    }
  }

  // Uninstall handlers from current thread
  crUninstallFromCurrentThread();
  ResetEvent(pInfo->m_hWakeUpEvent);

  // Exit this thread
  return 0;
}




