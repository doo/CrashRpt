#include "stdafx.h"
#include "smtpclient.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <Windns.h>
#include <atlenc.h>
#include <sys/stat.h>

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
 
  ResetEvent(scn->m_hEvent);

  HANDLE hThread = CreateThread(NULL, 0, SendThread, (void*)&ctx, 0, &dwThreadId);
  if(hThread==NULL)
    return 1;

  WaitForSingleObject(scn->m_hEvent, INFINITE);
  ResetEvent(scn->m_hEvent);

  return 0;
}

DWORD WINAPI CSmtpClient::SendThread(VOID* pParam)
{
  SendThreadContext* ctx = (SendThreadContext*)pParam;
  
  CEmailMessage* msg = ctx->m_msg;
  SmtpClientNotification* scn = ctx->m_scn;

  SetEvent(scn->m_hEvent); // signal that parameters are copied

  int nResult = _SendEmail(msg, scn);

  scn->m_cs.Lock();
  scn->m_nCompletionStatus = nResult;  
  scn->m_cs.Unlock();

  SetEvent(scn->m_hEvent);

  return nResult;
}


int CSmtpClient::_SendEmail(CEmailMessage* msg, SmtpClientNotification* scn)
{
  SetStatus(scn, _T("Start sending email"), 0, false);

  std::map<WORD, CString> host_list;

  int res = GetSmtpServerName(msg, false, host_list);
  if(res!=0)
    return 1;

  std::map<WORD, CString>::iterator it;
  for(it=host_list.begin(); it!=host_list.end(); it++)
  {
    if(0==SendEmailToRecipient(it->second, msg, scn))
    {
      return 0;
    }
  }

  return 1;
}

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

  return 1;
}

int CSmtpClient::SendEmailToRecipient(CString sSmtpServer, CEmailMessage* msg, SmtpClientNotification* scn)
{
  int status = 1;
  int iResult = -1;  
  CStringA sPostServer;
  struct addrinfo *result = NULL;
  struct addrinfo *ptr = NULL;
  struct addrinfo hints;
  CStringA sServiceName = "25";  
  SOCKET sock = INVALID_SOCKET;
  CStringA sMsg, str;
  std::set<CString>::iterator it;
  CString sStatusMsg;
  
  CString sMessageText = msg->m_sText;
  sMessageText.Replace(_T("\n"),_T("\r\n"));
  sMessageText.Replace(_T("\r\n.\r\n"),_T("\r\n*\r\n"));
  CString sUTF8Text = UTF16toUTF8(sMessageText);

  sStatusMsg.Format(_T("Getting address info of %s port %s"), sSmtpServer, sServiceName);
  SetStatus(scn, sStatusMsg, 1);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;  

  iResult = getaddrinfo(CStringA(sSmtpServer), sServiceName, &hints, &result);
  if(iResult!=0)
    goto exit;

  int res = SOCKET_ERROR;
  for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
  {
    sStatusMsg.Format(_T("Creating socket"));
    SetStatus(scn, sStatusMsg, 1);

    sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if(sock==INVALID_SOCKET)
      goto exit;
 
    sStatusMsg.Format(_T("Connecting to SMTP server %s port %s"), sSmtpServer, sServiceName);
    SetStatus(scn, sStatusMsg, 1);

    res = connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen);
    if(res!=SOCKET_ERROR)
      break;     
    
    closesocket(sock);
  }

  if(res==SOCKET_ERROR)
    goto exit;

  sStatusMsg.Format(_T("Connected OK"));
  SetStatus(scn, sStatusMsg, 5);

  char buf[1024]="";

  res = recv(sock, buf, 1024, 0);
  if(res==SOCKET_ERROR)
    goto exit;

  if(220!=GetMessageCode(buf)) 
    goto exit;

  char responce[1024];

  sStatusMsg.Format(_T("Sending HELO"));
  SetStatus(scn, sStatusMsg, 1);

  // send HELO
	res=SendMsg(sock, "HELO CrashSender\r\n", responce, 1024);
  if(res!=250)
    goto exit;
	
  sStatusMsg.Format(_T("Sending sender and recipient information"));
  SetStatus(scn, sStatusMsg, 1);

  sMsg.Format("MAIL FROM:<%s>\r\n", CStringA(msg->m_sFrom));
  res=SendMsg(sock, sMsg, responce, 1024);
  if(res!=250)
    goto exit;
	
  sMsg.Format("RCPT TO:<%s>\r\n", CStringA(msg->m_sTo));
  res=SendMsg(sock, sMsg, responce, 1024);
  if(res!=250)
  {
    goto exit;
  }

  sStatusMsg.Format(_T("Start sending email data"));
  SetStatus(scn, sStatusMsg, 1);

  // Send DATA
  res=SendMsg(sock, "DATA\r\n", responce, 1024);
  if(res!=354)
    goto exit;
  
  str.Format("From: <%s>\r\n", CStringA(msg->m_sFrom));
  sMsg  = str;
  str.Format("To: <%s>\r\n", CStringA(msg->m_sTo));
  sMsg += str;
  str.Format("Subject: %s\r\n", CStringA(msg->m_sSubject));
  sMsg += str;
  sMsg += "MIME-Version: 1.0\r\n";
  sMsg += "Content-Type: multipart/mixed; boundary=KkK170891tpbkKk__FV_KKKkkkjjwq\r\n";
  sMsg += "\r\n\r\n";
  res = SendMsg(sock, sMsg);
  if(res!=sMsg.GetLength())
    goto exit;

  /* Message text */

  sStatusMsg.Format(_T("Sending message text"));
  SetStatus(scn, sStatusMsg, 15);

  sMsg =  "--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n";
  sMsg += "Content-Type: text/plain; charset=UTF-8\r\n";
  sMsg += "\r\n";  
  sMsg += sUTF8Text;
  sMsg += "\r\n";
  res = SendMsg(sock, sMsg);
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
    res = SendMsg(sock, sMsg);
    if(res!=sMsg.GetLength())
      goto exit;
  
    // Encode data
    LPBYTE buf = NULL;
    int buf_len = 0;
    int nEncode=Base64EncodeAttachment(sFileName, &buf, buf_len);
    if(nEncode!=0)
      goto exit;

    // Send encoded data
    sMsg = CStringA((char*)buf, buf_len);        
    res = SendMsg(sock, sMsg);
    if(res!=sMsg.GetLength())
      goto exit;

    delete [] buf;
  }

  sMsg =  "\r\n--KkK170891tpbkKk__FV_KKKkkkjjwq--";
  res = SendMsg(sock, sMsg);
  if(res!=sMsg.GetLength())
    goto exit;

  // End of message marker
	if(250!=SendMsg(sock, "\r\n.\r\n", responce, 1024))
    goto exit;

	// quit
	if(221!=SendMsg(sock, "QUIT \r\n", responce, 1024))
    goto exit;

  // OK.
  status = 0;

exit:

  sStatusMsg.Format(_T("Finished"));
  SetStatus(scn, sStatusMsg, 100, false);

  // Clean up
  closesocket(sock);
  freeaddrinfo(result);  
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

CStringA CSmtpClient::UTF16toUTF8(const CStringW& utf16)
{
   CStringA utf8;
   int len = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, NULL, 0, 0, 0);
   if (len>1)
   { 
      char *ptr = utf8.GetBuffer(len-1);
      if (ptr) WideCharToMultiByte(CP_UTF8, 0, utf16, -1, ptr, len, 0, 0);
      utf8.ReleaseBuffer();
   }
   return utf8;
}

int CSmtpClient::SendMsg(SOCKET sock, PCSTR pszMessage, PSTR pszResponce, UINT uResponceSize)
{	
  int msg_len = (int)strlen(pszMessage);

  int res = send(sock, pszMessage, msg_len, 0);	
	if(pszResponce==NULL) 
    return res;

	int br = recv(sock, pszResponce, uResponceSize, 0);
  if(br==SOCKET_ERROR)
    return br;

  if(br>2)
	  pszResponce[br-2]=0; //zero terminate string
	
	return GetMessageCode(pszResponce);
}

int CSmtpClient::Base64EncodeAttachment(CString sFileName, 
    LPBYTE* ppEncodedFileData, int& nEncodedFileDataLen)
{
  *ppEncodedFileData = NULL;
  nEncodedFileDataLen = 0;

  int uFileSize = 0;
  BYTE* uchFileData = NULL;  
  struct _stat st;
  CStringA sFileNameA = CStringA(sFileName);

  int nResult = _stat(sFileNameA, &st);
  if(nResult != 0)
    return 1;  // File not found.
  
  // Allocate buffer of file size
  uFileSize = st.st_size;
  uchFileData = new BYTE[uFileSize];

  // Read file data to buffer.
  FILE* f = NULL;
  errno_t err = fopen_s(&f, sFileNameA, "rb");
  if(err!=0 || !f || fread(uchFileData, uFileSize, 1, f)!=1)
  {
    delete [] uchFileData;
    uchFileData = NULL;
    return 2; // Coudln't read file data.
  }
  
  fclose(f);

  
  DWORD dwFlags = ATL_BASE64_FLAG_NONE;
  int nEncodedLen = Base64EncodeGetRequiredLength(uFileSize, dwFlags);

  // Allocate buffer for encoded file data
  char* pchEncodedFileData = new char[nEncodedLen];    

  // Encode file data
  BOOL bEncoded = Base64Encode(uchFileData, uFileSize, pchEncodedFileData, 
      &nEncodedLen, dwFlags);
  if(!bEncoded)
  {
    delete [] pchEncodedFileData;
    return 3; // Couldn't BASE64 encode    
  }

  delete [] uchFileData;

  *ppEncodedFileData = (LPBYTE)pchEncodedFileData;
  nEncodedFileDataLen = nEncodedLen;

  // OK.
  return 0;
}

void CSmtpClient::SetStatus(SmtpClientNotification* scn, CString sStatusMsg, 
                            int percentCompleted, bool bRel)
{
  if(scn==NULL)
    return;

  scn->m_cs.Lock();
  
  scn->m_sStatusMsg = sStatusMsg;
  
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

