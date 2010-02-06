#pragma once
#include <set>
#include <map>
#include <vector>
#include <string>
#include "AssyncNotification.h"

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

  int SendEmail(CEmailMessage* msg);
  int SendEmailAssync(CEmailMessage* msg,  AssyncNotification* scn);

protected:

  static int _SendEmail(CEmailMessage* msg, AssyncNotification* scn);

  static int GetSmtpServerName(CEmailMessage* msg, AssyncNotification* scn, 
    std::map<WORD, CString>& host_list);
  
  static int SendEmailToRecipient(CString sSmtpServer, CEmailMessage* msg, 
    AssyncNotification* scn);
  
  static int GetMessageCode(LPSTR msg);
  
  static int CheckAddressSyntax(CString addr);
  
  static int SendMsg(AssyncNotification*, SOCKET sock, LPCTSTR pszMessage, LPSTR pszResponce=0, UINT uResponceSize=0);

  static int CheckAttachmentOK(CString sFileName);

  static int Base64EncodeAttachment(CString sFileName, 
	  std::string& sEncodedFileData);

  static std::string UTF16toUTF8(LPCWSTR utf16);
  
  static DWORD WINAPI SmtpSendThread(VOID* pParam);

  struct SmtpSendThreadContext
  {
    CEmailMessage* m_msg;
    AssyncNotification* m_scn;
  };
};



