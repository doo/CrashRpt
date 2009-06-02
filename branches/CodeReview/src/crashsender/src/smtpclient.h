#pragma once
#include "stdafx.h"
#include <atlstr.h>
#include <set>
#include <map>
#include <vector>

class CEmailMessage
{
public:
  CString m_sSubject;
  CString m_sFrom;
  CString m_sTo;
  CString m_sText;
  std::set<CString> m_aAttachments;
};

class CSmtpClient
{
public:
  
  CSmtpClient();
  ~CSmtpClient();

  int SendEmail(CEmailMessage& msg);

protected:

  int GetRecipientSmtp(CEmailMessage& msg, std::map<WORD, CString>& host_list);
  int SendEmailToRecipient(CString sSmtpServer, CEmailMessage& msg);
  int GetMessageCode(LPSTR msg);
  BOOL CheckAddressSyntax(CString addr);
  int SendMsg(SOCKET sock, PCSTR pszMessage, PSTR pszResponce=0, UINT uResponceSize=0);
};



