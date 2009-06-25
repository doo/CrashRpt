#include "stdafx.h"
#include "smtpclient.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/stat.h>
#include "base64.h"

#if _MSC_VER>=1300
#include <iphlpapi.h>
#include <Windns.h>
#else
#include <Ws2tcpip.h>
#endif


CSmtpClient::CSmtpClient()
{
  // Initialize Winsock
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  ATLASSERT(iResult==0); 
  iResult;
}

CSmtpClient::~CSmtpClient()
{
  int iResult = WSACleanup();
  iResult;
  ATLASSERT(iResult==0);
}

int CSmtpClient::SendEmail(CEmailMessage* msg)
{
  return _SendEmail(msg, NULL);
}

int CSmtpClient::SendEmailAssync(CEmailMessage* msg,  SmtpClientNotification* scn)
{
  DWORD dwThreadId = 0;
  SendThreadContext ctx;
  ctx.m_msg = msg;
  ctx.m_scn = scn;
 
  ResetEvent(scn->m_hCompletionEvent);

  HANDLE hThread = CreateThread(NULL, 0, SendThread, (void*)&ctx, 0, &dwThreadId);
  if(hThread==NULL)
    return 1;

  WaitForSingleObject(scn->m_hCompletionEvent, INFINITE);
  ResetEvent(scn->m_hCompletionEvent);
  
  return 0;
}

DWORD WINAPI CSmtpClient::SendThread(VOID* pParam)
{
  SendThreadContext* ctx = (SendThreadContext*)pParam;
  
  CEmailMessage* msg = ctx->m_msg;
  SmtpClientNotification* scn = ctx->m_scn;

  SetEvent(scn->m_hCompletionEvent); // signal that parameters have been copied

  int nResult = _SendEmail(msg, scn);

  scn->m_cs.Lock();
  scn->m_nCompletionStatus = nResult;  
  scn->m_cs.Unlock();

  SetEvent(scn->m_hCompletionEvent);

  return nResult;
}


int CSmtpClient::_SendEmail(CEmailMessage* msg, SmtpClientNotification* scn)
{
  SetStatus(scn, _T("Start sending email"), 0, false);

  std::map<WORD, CString> host_list;

  int res = GetSmtpServerName(msg, scn, host_list);
  if(res!=0)
  {
    SetStatus(scn, _T("Error querying DNS record."), 100, false);
    return 1;
  }

  std::map<WORD, CString>::iterator it;
  for(it=host_list.begin(); it!=host_list.end(); it++)
  {
    if(IsUserCancelled(scn))    
      return 2;

    int res = SendEmailToRecipient(it->second, msg, scn);
    if(res==0)
    {
      SetStatus(scn, _T("Finished OK."), 100, false);
      return 0;
    }
    if(res==2)
    {
      SetStatus(scn, _T("Critical error detected."), 100, false);
      return 2;
    }
  }

  SetStatus(scn, _T("Error sending email."), 100, false);

  return 1;
}

#if _MSC_VER>=1300
int CSmtpClient::GetSmtpServerName(CEmailMessage* msg, SmtpClientNotification* scn, 
                                   std::map<WORD, CString>& host_list)
{
  DNS_RECORD *apResult = NULL;

  CString sServer;
  sServer = msg->m_sTo.Mid(msg->m_sTo.Find('@')+1);
 
  CString sStatusMsg;
  sStatusMsg.Format(_T("Quering MX record of domain %s"), sServer);
  SetStatus(scn, sStatusMsg, 2);

  int r = DnsQuery(sServer, DNS_TYPE_MX, DNS_QUERY_STANDARD, 
    NULL, (PDNS_RECORD*)&apResult, NULL);
  
  if(r==0)
  {
    while(apResult!=NULL)
    {
      if(apResult->wType==DNS_TYPE_MX)        
      {
        host_list[apResult->Data.MX.wPreference] = 
        CStringW(apResult->Data.MX.pNameExchange);
      }

      apResult = apResult->pNext;
    }

    return 0;
  }

  sStatusMsg.Format(_T("DNS query failed with code %d"), r);
  SetStatus(scn, sStatusMsg, 2);
  return 1;
}
#else
int CSmtpClient::GetSmtpServerName(CEmailMessage* msg, SmtpClientNotification* scn, 
                                   std::map<WORD, CString>& host_list)
{
	return 1;
}

#endif

int CSmtpClient::SendEmailToRecipient(CString sSmtpServer, CEmailMessage* msg, SmtpClientNotification* scn)
{
  int status = 1;
  
  USES_CONVERSION;  

#if WINVER>=0x0500
  struct addrinfo *result = NULL;
  struct addrinfo *ptr = NULL;
  struct addrinfo hints;
#endif

  int iResult = -1;  
  CString sPostServer;
  CString sServiceName = "25";  
  SOCKET sock = INVALID_SOCKET;
  CString sMsg, str;
  std::set<CString>::iterator it;
  CString sStatusMsg;
  
  // Prepare message text
  CString sMessageText = msg->m_sText;
  sMessageText.Replace(_T("\n"),_T("\r\n"));
  sMessageText.Replace(_T("\r\n.\r\n"), _T("\r\n*\r\n"));
  LPWSTR lpwszMessageText = T2W(sMessageText.GetBuffer(0));
  std::string sUTF8Text = UTF16toUTF8(lpwszMessageText);

  // Check that all attachments exist
  for(it=msg->m_aAttachments.begin(); it!=msg->m_aAttachments.end(); it++)
  {
    if(CheckAttachmentOK(*it)!=0)
    {
      sStatusMsg.Format(_T("Attachment not found: %s"), *it);
      SetStatus(scn, sStatusMsg, 1);
      return 2; // critical error
    }
  }

  sStatusMsg.Format(_T("Getting address info of %s port %s"), sSmtpServer, CString(sServiceName));
  SetStatus(scn, sStatusMsg, 1);

  int res = SOCKET_ERROR;
  char buf[1024]="";
  std::string sEncodedFileData;

#if WINVER>=0x0500
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;  

  LPSTR lpszSmtpServer = T2A(sSmtpServer);
  LPSTR lpszServiceName = T2A(sServiceName);
  iResult = getaddrinfo(lpszSmtpServer, lpszServiceName, &hints, &result);
  if(iResult!=0)
    goto exit;
  
  for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
  {
    if(IsUserCancelled(scn)) {status = 2; goto exit;}

    sStatusMsg.Format(_T("Creating socket"));
    SetStatus(scn, sStatusMsg, 1);

    sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if(sock==INVALID_SOCKET)
    {
      SetStatus(scn, _T("Socket creation failed."), 1);
      goto exit;
    }
 
    sStatusMsg.Format(_T("Connecting to SMTP server %s port %s"), sSmtpServer, CString(sServiceName));
    SetStatus(scn, sStatusMsg, 1);

    res = connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen);
    if(res!=SOCKET_ERROR)
      break;     
    
    closesocket(sock);
  }
#endif

  if(res==SOCKET_ERROR)
    goto exit;

  sStatusMsg.Format(_T("Connected OK."));
  SetStatus(scn, sStatusMsg, 5);
  
  if(IsUserCancelled(scn)) {status = 2; goto exit;}

  res = recv(sock, buf, 1024, 0);
  if(res==SOCKET_ERROR)
  {
    sStatusMsg.Format(_T("Failed to receive greeting message from SMTP server (recv code %d)."), res);
    SetStatus(scn, sStatusMsg, 1);
    goto exit;
  }

  if(220!=GetMessageCode(buf)) 
  {
    goto exit;
  }


  char responce[1024];

  sStatusMsg.Format(_T("Sending HELO"));
  SetStatus(scn, sStatusMsg, 1);

  // send HELO
	res=SendMsg(scn, sock, _T("HELO CrashSender\r\n"), responce, 1024);
  if(res!=250)
    goto exit;
	
  sStatusMsg.Format(_T("Sending sender and recipient information"));
  SetStatus(scn, sStatusMsg, 1);
  
  sMsg.Format(_T("MAIL FROM:<%s>\r\n"), msg->m_sFrom);
  res=SendMsg(scn, sock, sMsg, responce, 1024);
  if(res!=250)
    goto exit;
	
  sMsg.Format(_T("RCPT TO:<%s>\r\n"), msg->m_sTo);
  res=SendMsg(scn, sock, sMsg, responce, 1024);
  if(res!=250)
  {
    goto exit;
  }

  sStatusMsg.Format(_T("Start sending email data"));
  SetStatus(scn, sStatusMsg, 1);

  // Send DATA
  res=SendMsg(scn, sock, _T("DATA\r\n"), responce, 1024);
  if(res!=354)
    goto exit;
    

  str.Format(_T("From: <%s>\r\n"), msg->m_sFrom);
  sMsg  = str;
  str.Format(_T("To: <%s>\r\n"), msg->m_sTo);
  sMsg += str;
  str.Format(_T("Subject: %s\r\n"), msg->m_sSubject);
  sMsg += str;
  sMsg += "MIME-Version: 1.0\r\n";
  sMsg += "Content-Type: multipart/mixed; boundary=KkK170891tpbkKk__FV_KKKkkkjjwq\r\n";
  sMsg += "\r\n\r\n";
  res = SendMsg(scn, sock, sMsg);
  if(res!=sMsg.GetLength())
    goto exit;

  /* Message text */

  sStatusMsg.Format(_T("Sending message text"));
  SetStatus(scn, sStatusMsg, 15);

  sMsg =  "--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n";
  sMsg += "Content-Type: text/plain; charset=UTF-8\r\n";
  sMsg += "\r\n";  
  sMsg += sUTF8Text.c_str();
  sMsg += "\r\n";
  res = SendMsg(scn, sock, sMsg);
  if(res!=sMsg.GetLength())
    goto exit;

  sStatusMsg.Format(_T("Sending attachments"));
  SetStatus(scn, sStatusMsg, 1);

  /* Attachments. */
  for(it=msg->m_aAttachments.begin(); it!=msg->m_aAttachments.end(); it++)
  {    
    CString sFileName = *it;
    sFileName.Replace('/', '\\');
    CString sDisplayName = sFileName.Mid(sFileName.ReverseFind('\\')+1);

    // Header
    sMsg =  "\r\n--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n";
    sMsg += "Content-Type: application/octet-stream\r\n";
    sMsg += "Content-Transfer-Encoding: base64\r\n";
    sMsg += "Content-Disposition: attachment; filename=\"";
    sMsg += sDisplayName;
    sMsg += "\"\r\n";
    sMsg += "\r\n";
    res = SendMsg(scn, sock, sMsg);
    if(res!=sMsg.GetLength())
      goto exit;
  
    // Encode data
    LPBYTE buf = NULL;
    //int buf_len = 0;
    int nEncode=Base64EncodeAttachment(sFileName, sEncodedFileData);
    if(nEncode!=0)
    {
      sStatusMsg.Format(_T("Error BASE64-encoding attachment %s"), sFileName);
      SetStatus(scn, sStatusMsg, 1);
      goto exit;
    }

    // Send encoded data
    sMsg = sEncodedFileData.c_str();        
    res = SendMsg(scn, sock, sMsg);
    if(res!=sMsg.GetLength())
      goto exit;

    delete [] buf;
  }

  sMsg =  "\r\n--KkK170891tpbkKk__FV_KKKkkkjjwq--";
  res = SendMsg(scn, sock, sMsg);
  if(res!=sMsg.GetLength())
    goto exit;

  // End of message marker
	if(250!=SendMsg(scn, sock, _T("\r\n.\r\n"), responce, 1024))
    goto exit;

	// quit
	if(221!=SendMsg(scn, sock, _T("QUIT \r\n"), responce, 1024))
    goto exit;

  // OK.
  status = 0;

exit:

  if(IsUserCancelled(scn)) 
    status = 2;

  sStatusMsg.Format(_T("Finished with error code %d"), status);
  SetStatus(scn, sStatusMsg, 100, false);

  // Clean up
  closesocket(sock);
#if WINVER>=0x0500
  freeaddrinfo(result);  
#endif
  return status;
}

int CSmtpClient::GetMessageCode(LPSTR msg)
{
  if(msg==NULL)
    return -1;

	return atoi(msg);
}

int CSmtpClient::CheckAddressSyntax(CString addr)
{
	if(addr=="") return FALSE;
	return TRUE;
}

std::string CSmtpClient::UTF16toUTF8(LPCWSTR utf16)
{
   std::string utf8;
   int len = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, 0, 0);
   if (len>1)
   { 
	  char* buf = new char[len+1];	  
      WideCharToMultiByte(CP_UTF8, 0, utf16, -1, buf, len, 0, 0);
      utf8 = buf;
	  delete [] buf;
   }
   
   return utf8;
}

int CSmtpClient::SendMsg(SmtpClientNotification* scn, SOCKET sock, LPCTSTR pszMessage, LPSTR pszResponce, UINT uResponceSize)
{	
  USES_CONVERSION;

  if(IsUserCancelled(scn)) {return -1;}

  int msg_len = (int)_tcslen(pszMessage);

  LPSTR lpszMessageA = T2A((TCHAR*)pszMessage);
  
  int res = send(sock, lpszMessageA, msg_len, 0);	
	if(pszResponce==NULL) 
    return res;

	int br = recv(sock, pszResponce, uResponceSize, 0);
  if(br==SOCKET_ERROR)
    return br;

  if(br>2)
	  pszResponce[br-2]=0; //zero terminate string
	
	return GetMessageCode(pszResponce);
}

int CSmtpClient::CheckAttachmentOK(CString sFileName)
{
  USES_CONVERSION;

  struct _stat st;
  LPSTR lpszFileNameA = T2A(sFileName.GetBuffer(0));

  int nResult = _stat(lpszFileNameA, &st);
  if(nResult != 0)
    return 1;  // File not found.

  return 0;
}

int CSmtpClient::Base64EncodeAttachment(CString sFileName, 
										std::string& sEncodedFileData)
{
  USES_CONVERSION;
  
  int uFileSize = 0;
  BYTE* uchFileData = NULL;  
  struct _stat st;
  LPSTR lpszFileNameA = T2A(sFileName.GetBuffer(0));

  int nResult = _stat(lpszFileNameA, &st);
  if(nResult != 0)
    return 1;  // File not found.
  
  // Allocate buffer of file size
  uFileSize = st.st_size;
  uchFileData = new BYTE[uFileSize];

  // Read file data to buffer.
  FILE* f = NULL;
#if _MSC_VER<1400
  f = fopen(lpszFileNameA, "rb");
#else
  /*errno_t err = */_tfopen_s(&f, sFileName, _T("rb"));  
#endif 

  if(!f || fread(uchFileData, uFileSize, 1, f)!=1)
  {
    delete [] uchFileData;
    uchFileData = NULL;
    return 2; // Coudln't read file data.
  }
  
  fclose(f);
  
  sEncodedFileData = base64_encode(uchFileData, uFileSize);

  delete [] uchFileData;

  // OK.
  return 0;
}

void CSmtpClient::SetStatus(SmtpClientNotification* scn, CString sStatusMsg, 
                            int percentCompleted, bool bRel)
{
  if(scn==NULL)
    return;

  scn->m_cs.Lock();
  
  scn->m_statusLog.push_back(sStatusMsg);
  
  if(bRel)
  {
    scn->m_nPercentCompleted += percentCompleted;
    if(scn->m_nPercentCompleted>100)
    {
      scn->m_nPercentCompleted = 100;
    }
  }
  else
    scn->m_nPercentCompleted = percentCompleted;

  scn->m_cs.Unlock();
} 

bool CSmtpClient::IsUserCancelled(SmtpClientNotification* scn)
{
  DWORD dwWaitResult = WaitForSingleObject(scn->m_hCancelEvent, 0);
  if(dwWaitResult==WAIT_OBJECT_0)
  {
    SetEvent(scn->m_hCancelEvent);
    SetStatus(scn, _T("Cancelled by user."), 0);
    return true;
  }
  
  return false;
}

