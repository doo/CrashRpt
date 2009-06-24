#pragma once
#include "stdafx.h"
#include <vector>

struct SenderThreadContext
{  
  CComAutoCriticalSection m_cs;
  int m_nStatus;
  int m_nProgressPct;
  std::vector<CString> m_Messages;
  HANDLE m_hCancelEvent;

  CString m_sEmailFrom;
  CString m_sEmailTo;
  CString m_sEmailSubject;
  CString m_sEmailText;
  CString m_sUrl;
  CString m_sZipName;
  UINT m_uPriorities[3];
};

BOOL GetSenderThreadStatus(int& nProgressPct, std::vector<CString>& msg_log);
DWORD WINAPI SenderThread(LPVOID lpParam);

