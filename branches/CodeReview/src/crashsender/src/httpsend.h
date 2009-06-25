#pragma once
#include "stdafx.h"
#include <string>

struct HttpSendParams
{
  CString m_sURL;
  CString m_sFileName;
  HANDLE m_hCompletionEvent;
  int m_nCompletionStatus;
};

class CHttpSender
{
public:
  
  BOOL SendAssync(HttpSendParams* params);

private:

  static void _str_replace(std::string& str, char* charToReplace, char* strToInsert);

  static void ParseURL(LPCTSTR szURL, LPTSTR szProtocol, UINT cbProtocol,
    LPTSTR szAddress, UINT cbAddress, DWORD &dwPort, LPTSTR szURI, UINT cbURI);

  static BOOL _Send(CString sURL, CString sFileName);

  static DWORD WINAPI HttpSendThread(VOID* pParam);

  
};


