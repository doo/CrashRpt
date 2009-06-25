#pragma once
#include "stdafx.h"
#include <set>
#include <map>
#include <vector>
#include <string>

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
    m_hCompletionEvent = NULL;
    m_hCancelEvent = NULL;
    m_nPercentCompleted = 0;
  }

  CComAutoCriticalSection m_cs;
  int m_nCompletionStatus;
  HANDLE m_hCompletionEvent;
  HANDLE m_hCancelEvent;
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
  
  static int SendMsg(SmtpClientNotification*, SOCKET sock, LPCTSTR pszMessage, LPSTR pszResponce=0, UINT uResponceSize=0);

  static int CheckAttachmentOK(CString sFileName);

  static int Base64EncodeAttachment(CString sFileName, 
	  std::string& sEncodedFileData);

  static std::string UTF16toUTF8(LPCWSTR utf16);

  static bool IsUserCancelled(SmtpClientNotification* scn);

  static void SetStatus(SmtpClientNotification* scn, CString sStatusMsg, 
    int percentCompleted, bool bRelative=true);

  static DWORD WINAPI SendThread(VOID* pParam);

  struct SendThreadContext
  {
    CEmailMessage* m_msg;
    SmtpClientNotification* m_scn;
  };
};



