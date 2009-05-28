#pragma once
#include "stdafx.h"
#include "CrashRpt.h"


struct CrashThreadInfo
{
  LPVOID m_pCrashRptState;
  HANDLE m_hWakeUpEvent;
  int m_ExceptionType;
};

DWORD WINAPI CrashThread(LPVOID pParam);



