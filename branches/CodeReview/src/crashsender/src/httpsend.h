#pragma once
#include "stdafx.h"

class CHttpSender
{
public:

  BOOL Send(CString sURL, CString sFileName);
  void ParseURL(LPCTSTR szURL, LPTSTR szProtocol, UINT cbProtocol,
    LPTSTR szAddress, UINT cbAddress, DWORD &dwPort, LPTSTR szURI, UINT cbURI);

private:

};


