#include "stdafx.h"
#include "SenderThread.h"
#include "MailMsg.h"
#include "smtpclient.h"
#include "httpsend.h"

int stage = 0;
CEmailMessage msg;
SmtpClientNotification scn;
CSmtpClient smtp;  

HttpSendParams hp;
CHttpSender http;

CMailMsg mailmsg;

BOOL GetSenderThreadStatus(int& nProgressPct, std::vector<CString>& msg_log)
{
  if(stage==1)
  {
  }

  if(stage==2)
  {
    scn.m_cs.Lock();
    nProgressPct = scn.m_nPercentCompleted;
    msg_log = scn.m_statusLog;
    scn.m_statusLog.clear();
    scn.m_cs.Unlock();
  }

  return FALSE;
}

void CancelSenderThread()
{
  SetEvent(scn.m_hCancelEvent);
}

BOOL SendOverHTTP(SenderThreadContext* pc)
{
  stage = 1;

  hp.m_sFileName = pc->m_sZipName;
  hp.m_sURL = pc->m_sUrl;
  
  CHttpSender httpsender;
  BOOL bSend = httpsender.SendAssync(&hp);
  return bSend;
}

BOOL SendOverSMTP(SenderThreadContext* pc, SmtpClientNotification* pscn)
{
  // Send over SMTP

  stage = 2;
  
  msg.m_sFrom = pc->m_sEmailFrom;
  msg.m_sTo = pc->m_sEmailTo;
  msg.m_sSubject = pc->m_sEmailSubject;
  msg.m_sText = pc->m_sEmailText;
  msg.m_aAttachments.insert(pc->m_sZipName);  
  int res = smtp.SendEmailAssync(&msg, pscn);

  return (res==0);
}

BOOL SendOverSMAPI(SenderThreadContext* pc)
{
  stage = 3;
  
  mailmsg.SetFrom(pc->m_sEmailFrom);
  mailmsg.SetTo(pc->m_sEmailTo);
  mailmsg.SetSubject(pc->m_sEmailSubject);
  CString sFileTitle = pc->m_sZipName;
  sFileTitle.Replace('/', '\\');
  int pos = sFileTitle.ReverseFind('\\');
  if(pos>=0)
    sFileTitle = sFileTitle.Mid(pos+1);
  mailmsg.SetMessage(pc->m_sEmailText);
  mailmsg.AddAttachment(pc->m_sZipName, sFileTitle);
  BOOL bSend = mailmsg.Send();

  return bSend;
}

DWORD WINAPI SenderThread(LPVOID lpParam)
{
  SenderThreadContext* pc = (SenderThreadContext*)lpParam;
  
  scn.m_hCompletionEvent = CreateEvent(0, FALSE, FALSE, 0);
  scn.m_hCancelEvent = CreateEvent(0, FALSE, FALSE, 0);

  hp.m_hCompletionEvent = CreateEvent(0, FALSE, FALSE, 0);

  /*SendOverHTTP(pc);  
  WaitForSingleObject(hp.m_hCompletionEvent, INFINITE);

  if(hp.m_nCompletionStatus==0)
    goto exit;

  SendOverSMTP(pc, &scn);  
  WaitForSingleObject(scn.m_hCompletionEvent, INFINITE);

  if(scn.m_nCompletionStatus==0)
    goto exit;*/

  BOOL bMAPIInit = mailmsg.MAPIInitialize();
  
  BOOL bSend = SendOverSMAPI(pc);

exit:

  return 0;
}


