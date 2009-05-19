#pragma once
#include "stdafx.h"
#include "CrashRpt.h"

enum eExceptionType
{
  ET_NO_EXCEPTION, // exit without exception
  ET_UNHANDLED_WIN32_EXCEPTION, 
  ET_CPP_TERMINATE,
  ET_CPP_UNEXPECTED,
  ET_CPP_PURECALL,
  ET_CPP_SECURITY,
  ET_CPP_INVALIDPARAM,
  ET_CPP_NEW,
  ET_CPP_SIGABRT,
  ET_CPP_SIGFPE,
  ET_CPP_SIGILL,
  ET_CPP_SIGINT,
  ET_CPP_SIGSEGV,
  ET_CPP_SIGTERM
};

struct CrashThreadInfo
{
  LPVOID m_pCrashRptState;
  HANDLE m_hWakeUpEvent;
  eExceptionType m_ExceptionType;
};

int GenerateException(eExceptionType ExceptionType);

DWORD WINAPI CrashThread(LPVOID pParam);



