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
   CCrashHandler *pImpl = new CCrashHandler(pfn, lpcszTo, lpcszSubject);
   CRASH_ASSERT(pImpl);

   return pImpl;
}

// Sets C++ exception handlers for the calling thread
CRASHRPTAPI void crInstallToCurrentThread(LPVOID lpState)
{
  CCrashHandler *pImpl = (CCrashHandler*)lpState;
  CRASH_ASSERT(pImpl);

  pImpl->SetThreadCPPExceptionHandlers();
}

// Unsets C++ exception handlers from the calling thread
CRASHRPTAPI void crUninstallFromCurrentThread(LPVOID lpState)
{
  CCrashHandler *pImpl = (CCrashHandler*)lpState;
  CRASH_ASSERT(pImpl);

  pImpl->UnSetThreadCPPExceptionHandlers();
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