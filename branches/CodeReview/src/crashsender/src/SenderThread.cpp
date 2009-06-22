#include "stdafx.h"
#include "SenderThread.h"
#include "MailMsg.h"
#include "smtpclient.h"
#include "httpsend.h"

void SetProgressPct(SenderThreadContext* pc, int n)
{
  pc->m_cs.Lock();
  pc->m_nProgressPct = n;
  pc->m_cs.Unlock();
}

void AddStatusMsg(SenderThreadContext* pc, CString msg)
{
  pc->m_cs.Lock();
  pc->m_Messages.push_back(msg);
  pc->m_cs.Unlock();
}


DWORD WINAPI SenderThread(LPVOID lpParam)
{
  SenderThreadContext* pc = (SenderThreadContext*)lpParam;

  //// Send over HTTP

  //CHttpSender httpsender;
  //BOOL bSend = httpsender.Send(pc->m_sUrl, pc->m_sZipName);

  // Send over SMTP

  /*CEmailMessage msg;
  msg.m_sFrom = pc->m_sEmailFrom;
  msg.m_sTo = pc->m_sEmailTo;
  msg.m_sSubject = pc->m_sEmailSubject;
  msg.m_sText = pc->m_sProblemDescription;
  msg.m_aAttachments.insert(pc->m_sZipName);
  CSmtpClient smtp;
  CSmtpClientNotification scn;
  int res = smtp.SendEmailAssync(msg, scn);*/

  // Send over MAPI

  AddStatusMsg(pc, _T("Sending over MAPI"));
  SetProgressPct(pc, 2);

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

  AddStatusMsg(pc, _T("Completed"));
  SetProgressPct(pc, 100);

  

  return 0;
}


