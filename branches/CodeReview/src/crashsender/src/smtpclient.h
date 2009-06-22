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

struct SmtpClientNotification
{
  SmtpClientNotification()
  {
    m_nCompletionStatus = -1;    
    m_hEvent = NULL;
    m_nPercentCompleted = 0;
  }

  CComAutoCriticalSection m_cs;
  int m_nCompletionStatus;
  HANDLE m_hEvent;
  int m_nPercentCompleted;
  std::vector<CString> m_statusLog;
};

class CSmtpClient
{
public:
  
  CSmtpClient();
  ~CSmtpClient();

  int SendEmail(CEmailMessage* msg);
  int SendEmailAssync(CEmailMessage* msg,  SmtpClientNotification* scn);

protected:

  static int _SendEmail(CEmailMessage* msg, SmtpClientNotification* scn);

  static int GetSmtpServerName(CEmailMessage* msg, SmtpClientNotification* scn, 
    std::map<WORD, CString>& host_list);
  
  static int SendEmailToRecipient(CString sSmtpServer, CEmailMessage* msg, 
    SmtpClientNotification* scn);
  
  static int GetMessageCode(LPSTR msg);
  
  static int CheckAddressSyntax(CString addr);
  
  static int SendMsg(SOCKET sock, PCSTR pszMessage, PSTR pszResponce=0, UINT uResponceSize=0);

  static int Base64EncodeAttachment(CString sFileName, 
    LPBYTE* ppEncodedFileData, int& nEncodedFileDataLen);

  static CStringA UTF16toUTF8(const CStringW& utf16);

  static void SetStatus(SmtpClientNotification* scn, CString sStatusMsg, 
    int percentCompleted, bool bRelative=true);

  static DWORD WINAPI SendThread(VOID* pParam);

  struct SendThreadContext
  {
    CEmailMessage* m_msg;
    SmtpClientNotification* m_scn;
  };
};



