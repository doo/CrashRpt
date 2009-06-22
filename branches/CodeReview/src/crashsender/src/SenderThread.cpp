#include "stdafx.h"
#include "SenderThread.h"
#include "MailMsg.h"
#include "smtpclient.h"
#include "httpsend.h"

int stage = 0;
CEmailMessage msg;
SmtpClientNotification scn;
CSmtpClient smtp;  

BOOL GetSenderThreadStatus(SenderThreadContext* pc, int& nProgressPct, std::vector<CString>& msg_log)
{
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

BOOL SendOverHTTP(SenderThreadContext* pc)
{
  CHttpSender httpsender;
  BOOL bSend = httpsender.Send(pc->m_sUrl, pc->m_sZipName);
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
  CMailMsg mailmsg;
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
  
  scn.m_hEvent = CreateEvent(0, FALSE, FALSE, 0);
  SendOverSMTP(pc, &scn);  

  WaitForSingleObject(scn.m_hEvent, INFINITE);

  return 0;
}


