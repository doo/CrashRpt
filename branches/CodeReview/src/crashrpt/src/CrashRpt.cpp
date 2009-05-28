///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashRpt.cpp
//
//    Desc: See CrashRpt.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CrashRpt.h"
#include "CrashHandler.h"

#ifdef _DEBUG
#define CRASH_ASSERT(pObj)          \
   if (!pObj || sizeof(*pObj) != sizeof(CCrashHandler))  \
      DebugBreak()                                       
#else
#define CRASH_ASSERT(pObj)
#endif // _DEBUG

CRASHRPTAPI LPVOID Install(LPGETLOGFILE pfn, LPCTSTR lpcszTo, LPCTSTR lpcszSubject)
{
   CCrashHandler *pImpl = new CCrashHandler();
   pImpl->Init(0, 0, pfn, lpcszTo, lpcszSubject);
   CRASH_ASSERT(pImpl);

   return pImpl;
}

CRASHRPTAPI void Uninstall(LPVOID lpState)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   delete pImpl;
}

CRASHRPTAPI void AddFile(LPVOID lpState, LPCTSTR lpFile, LPCTSTR lpDesc)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   pImpl->AddFile(lpFile, lpDesc);
}

CRASHRPTAPI void GenerateErrorReport(LPVOID lpState, PEXCEPTION_POINTERS pExInfo)
{
   CCrashHandler *pImpl = (CCrashHandler*)lpState;
   CRASH_ASSERT(pImpl);

   pImpl->GenerateErrorReport(pExInfo);
}

int crInstall(PCRASHRPT_INFO pInfo, LPVOID* ppInstance)
{
  *ppInstance = NULL;

  // Validate input parameters.
  if(pInfo==NULL || 
     pInfo->cb!=sizeof(CRASHRPT_INFO) ||
     ppInstance == NULL)
  {
    ATLASSERT(pInfo->cb==sizeof(CRASHRPT_INFO));
    ATLASSERT(pInfo != NULL);
    ATLASSERT(ppInstance != NULL);
    return 1; // Invalid parameter?
  }

  CCrashHandler *pImpl = new CCrashHandler();
  if(pImpl==NULL)
  {
    ATLASSERT(pImpl!=NULL);
    return 2; // Memory allocation error?
  }

  int nInitResult = pImpl->Init(
    pInfo->pszAppName, 
    pInfo->pszAppVersion, 
    pInfo->pfnCrashCallback,
    pInfo->pszEmailTo,
    pInfo->pszEmailSubject);
  
  if(nInitResult!=0)
  {
    ATLASSERT(nInitResult==0);
    // Crash handler initialization error?
  }

  *ppInstance = (LPVOID)pImpl;

  // OK.
  return 0;
}

int crUninstall(LPVOID lpState)
{
  CCrashHandler *pImpl = (CCrashHandler*)lpState;
  if(pImpl==NULL)
  {
    ATLASSERT(pImpl!=NULL);
    return 1; // Invalid parameter?
  }

  delete pImpl;

  // OK.
  return 0;
}

// Sets C++ exception handlers for the calling thread
CRASHRPTAPI int crInstallToCurrentThread(LPVOID lpState)
{
  CCrashHandler *pImpl = (CCrashHandler*)lpState;
  if(pImpl==NULL)
  {
    ATLASSERT(pImpl!=NULL);
    return 1; // Invalid parameter?
  }

  int nResult = pImpl->SetThreadCPPExceptionHandlers();
  if(nResult!=0)
    return 2; // Error?

  // Ok.
  return 0;
}

// Unsets C++ exception handlers from the calling thread
CRASHRPTAPI int crUninstallFromCurrentThread(LPVOID lpState)
{
  CCrashHandler *pImpl = (CCrashHandler*)lpState;
  if(pImpl==NULL)
  {
    ATLASSERT(pImpl!=NULL);
    return 1; // Invalid parameter?
  }

  int nResult = pImpl->UnSetThreadCPPExceptionHandlers();
  if(nResult!=0)
    return 2; // Error?

  // OK.
  return 0;
}
