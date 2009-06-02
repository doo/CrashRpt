#include "stdafx.h"
#include "smtpclient.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <Windns.h>


CSmtpClient::CSmtpClient()
{

}

CSmtpClient::~CSmtpClient()
{

}

int CSmtpClient::SendEmail(CEmailMessage& msg)
{
  std::map<WORD, CString> host_list;

  int res = GetRecipientSmtp(msg, host_list);
  if(res!=0)
    return 1;

  std::map<WORD, CString>::iterator it;
  for(it=host_list.begin(); it!=host_list.end(); it++)
  {
    if(0==SendEmailToRecipient(it->second, msg))
    {
      return 0;
    }
  }

  return 1;
}

int CSmtpClient::GetRecipientSmtp(CEmailMessage& msg, std::map<WORD, CString>& host_list)
{
  DNS_RECORD *apResult = NULL;

  CString sServer = msg.m_sTo.Mid(msg.m_sTo.Find('@')+1);

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
  else
  {
  }

  return 1;
}

int CSmtpClient::SendEmailToRecipient(CString sSmtpServer, CEmailMessage& msg)
{
  int status = 1;
  int iResult = -1;
  WSADATA wsaData;
  CStringA sPostServer;
  struct addrinfo *result = NULL;
  struct addrinfo *ptr = NULL;
  struct addrinfo hints;
  CStringA sServiceName = "25";  
  SOCKET sock = INVALID_SOCKET;
  CStringA sMsg;
  
  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) 
  {      
    return 1;
  }
    
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
    sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if(sock==INVALID_SOCKET)
      goto exit;
 
    res = connect(sock, ptr->ai_addr, (int)ptr->ai_addrlen);
    if(res!=SOCKET_ERROR)
      break;     
    
    closesocket(sock);
  }

  if(res==SOCKET_ERROR)
    goto exit;

  char buf[1024]="";

  res = recv(sock, buf, 1024, 0);
  if(res==SOCKET_ERROR)
    goto exit;

  if(220!=GetMessageCode(buf)) 
    goto exit;

  char responce[1024];

  // send HELO
	res=SendMsg(sock, "HELO CrashSender\r\n", responce, 1024);
  if(res!=250)
    goto exit;
	
  sMsg.Format("MAIL FROM:<%s>\r\n", CStringA(msg.m_sFrom));
  res=SendMsg(sock, sMsg, responce, 1024);
  if(res!=250)
    goto exit;
	
  sMsg.Format("RCPT TO:<%s>\r\n", CStringA(msg.m_sTo));
  res=SendMsg(sock, sMsg, responce, 1024);
  if(res!=250)
    goto exit;

  // Send DATA
  res=SendMsg(sock, "DATA\r\n", responce, 1024);
  if(res!=354)
    goto exit;

  // send message header
	//sMsg.Format("Date: %s\n",cur_time.Format("%x %X"));
	//SendMsg(sockClient,msg,FALSE);

  sMsg.Format("From: <%s>\r\n", CStringA(msg.m_sFrom));
	res = SendMsg(sock, sMsg);

  sMsg.Format("To: <%s>\r\n", CStringA(msg.m_sTo));
	res = SendMsg(sock, sMsg);
	
  sMsg.Format("Subject: %s\r\n", CStringA(msg.m_sSubject));
	res = SendMsg(sock, sMsg);

  sMsg = "MIME-Version: 1.0\r\n";
  sMsg += "Content-Type: multipart/mixed;\r\n";
  sMsg += " boundary= \"KkK170891tpbkKk__FV_KKKkkkjjwq\"\r\n";
  sMsg += "\r\n";
  sMsg += "--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n";
  sMsg += "Content-Type: text/plain; charset=US-ASCII\r\n";
  sMsg += "\r\n";  
  sMsg += msg.m_sText;
  sMsg += "\r\n";
  sMsg += "--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n";
  res = SendMsg(sock, sMsg);

  sMsg = "Content-Type: application/octet-stream\r\n";
  sMsg += "Content-Transfer-Encoding: base64\r\n";
  sMsg += "Content-Disposition: attachment;\r\n";
  sMsg += " filename= \"file.txt\"\r\n";
  sMsg += "\r\n";
  sMsg += "Here goes the Base64 encoded attachment\r\n";
  sMsg += "\r\n";
  sMsg += "--KkK170891tpbkKk__FV_KKKkkkjjwq\r\n";
  res = SendMsg(sock, sMsg);

  // End of message marker
	if(250!=SendMsg(sock, "\r\n.\r\n", responce, 1024))
    goto exit;

	// quit
	if(221!=SendMsg(sock, "QUIT \r\n", responce, 1024))
    goto exit;

  // OK.
  status = 0;

exit:

  // Clean up
  closesocket(sock);
  freeaddrinfo(result);
  WSACleanup();
  return status;
}

int CSmtpClient::GetMessageCode(LPSTR msg)
{
  if(msg==NULL)
    return -1;

	return atoi(msg);
}

BOOL CSmtpClient::CheckAddressSyntax(CString addr)
{
	if(addr=="") return FALSE;
	return TRUE;
}

int CSmtpClient::SendMsg(SOCKET sock, PCSTR pszMessage, PSTR pszResponce, UINT uResponceSize)
{	
  int msg_len = strlen(pszMessage);

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

