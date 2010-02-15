/************************************************************************************* 
  This file is a part of CrashRpt library.

  CrashRpt is Copyright (c) 2003, Michael Carruth
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:
 
   * Redistributions of source code must retain the above copyright notice, this 
     list of conditions and the following disclaimer.
 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.
 
   * Neither the name of the author nor the names of its contributors 
     may be used to endorse or promote products derived from this software without 
     specific prior written permission.
 

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

#include "stdafx.h"
#include "HttpRequestSender.h"
#include <wininet.h>
#include <sys/stat.h>
#include "Base64.hpp"
#include "md5.h"
#include <string>
#include "Utility.h"
#include "strconv.h"

#define PART_BOUNDARY _T("AaB03x")

BOOL CHttpRequestSender::SendAssync(CHttpRequest& Request, AssyncNotification* an)
{
  // Copy parameters
  m_Request = Request;
  m_Assync = an;

  // Create worker thread
  HANDLE hThread = CreateThread(NULL, 0, WorkerThread, (void*)this, 0, NULL);
  if(hThread==NULL)
    return FALSE;
  
  return TRUE;
}

DWORD WINAPI CHttpRequestSender::WorkerThread(VOID* pParam)
{
  CHttpRequestSender* pSender = (CHttpRequestSender*)pParam;
  // Delegate further actions to CHttpRequestSender class
  pSender->InternalSend();  

  return 0;
}

BOOL CHttpRequestSender::InternalSend()
{ 
//  strconv_t strconv;
//  BOOL bStatus = FALSE;
//	TCHAR* hdrs = _T("Content-Type: application/x-www-form-urlencoded");
//	LPCTSTR accept[2]={_T("*/*"), NULL};
//  int uFileSize = 0;
//  BYTE* uchFileData = NULL;
//  TCHAR szProtocol[512];
//  TCHAR szServer[512];
//  TCHAR szURI[1024];
//  DWORD dwPort;
//  struct _stat st;
//  int res = -1;
//  FILE* f = NULL;
//  BOOL bResult = FALSE;
//  char* chPOSTRequest = NULL;
//  CString sMD5Hash;
//  CString sPOSTRequest;
//  LPCSTR szPOSTRequest; // ASCII
//  char* szPrefix="crashrpt=\"";
//  char* szSuffix="\"";
//  CString sErrorMsg;
//  CHAR szResponce[1024];
//  DWORD dwBufSize = 1024;
//  MD5 md5;
//  MD5_CTX md5_ctx;
//  unsigned char md5_hash[16];
//  int i=0;  
//  CString msg; 
//  CString sFileName;
//
//  m_Assync->SetProgress(_T("Start sending error report over HTTP"), 0, false);
//
//  m_Assync->SetProgress(_T("Creating Internet connection"), 3, false);
//
//  if(m_Assync->IsCancelled()){ goto exit; }
//
//  // Create Internet session
//	hSession = InternetOpen(_T("CrashRpt"),
//		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
//	if(hSession==NULL)
//  {
//    m_Assync->SetProgress(_T("Error creating Internet conection"), 0);
//	  goto exit; // Couldn't create internet session
//  }
//  
//  ParseURL(m_Request.m_sUrl, szProtocol, 512, szServer, 512, dwPort, szURI, 1024);
//
//  m_Assync->SetProgress(_T("Connecting to server"), 5, false);
//
//  // Connect to server
//	hConnect = InternetConnect(
//    hSession, 
//    szServer,
//		INTERNET_DEFAULT_HTTP_PORT, 
//    NULL, 
//    NULL, 
//    INTERNET_SERVICE_HTTP, 
//    0, 
//    1);
//	
//	if(hConnect==NULL)
//  {
//    m_Assync->SetProgress(_T("Error connecting to server"), 0);
//	  goto exit; // Couldn't connect
//  }
//	
//  if(m_Assync->IsCancelled()){ goto exit; }
//
//  m_Assync->SetProgress(_T("Preparing HTTP request data"), 7, false);
//
//  // Load file data into memory
//  res = _tstat(sFileName.GetBuffer(0), &st);
//  if(res!=0)
//  {
//    m_Assync->SetProgress(_T("Error opening file"), 0);
//    goto exit; // File not found
//  }
//  
//  uFileSize = st.st_size;
//  uchFileData = new BYTE[uFileSize];
//#if _MSC_VER<1400
//  f = _tfopen(sFileName.GetBuffer(0), _T("rb"));
//#else
//  _tfopen_s(&f, sFileName.GetBuffer(0), _T("rb"));
//#endif
//  if(!f || fread(uchFileData, uFileSize, 1, f)!=1)
//  {
//    m_Assync->SetProgress(_T("Error reading file"), 0);
//    goto exit;  
//  }
//  fclose(f);
//
//  md5.MD5Init(&md5_ctx);
//  md5.MD5Update(&md5_ctx, uchFileData, uFileSize);
//  md5.MD5Final(md5_hash, &md5_ctx);
//  
//  sMD5Hash = _T("&md5=");
//  for(i=0; i<16; i++)
//  {
//    CString number;
//    number.Format(_T("%02X"), md5_hash[i]);
//    sMD5Hash += number;
//  }
//  
//  //sPOSTRequest = base64_encode(uchFileData, uFileSize).c_str();
//  sPOSTRequest = szPrefix + sPOSTRequest + szSuffix;  
//  sPOSTRequest.Replace(_T("+"), _T("%2B"));
//  sPOSTRequest.Replace(_T("/"), _T("%2F"));  
//
//  sPOSTRequest += sMD5Hash;
//  
//  m_Assync->SetProgress(_T("Opening HTTP request"), 10);
//
//  if(m_Assync->IsCancelled()){ goto exit; }
//
//  // Send POST request
//  hRequest = HttpOpenRequest(hConnect, _T("POST"),
//		                         szURI, NULL, NULL, accept, 0, 1);	
//	if(hRequest==NULL)
//  {
//    m_Assync->SetProgress(_T("Error opening HTTP request"), 0);
//	  goto exit; // Coudn't open request	
//  }
//
//  if(m_Assync->IsCancelled()){ goto exit; }
//
//  szPOSTRequest = strconv.t2a(sPOSTRequest.GetBuffer(0));
//
//  m_Assync->SetProgress(_T("Sending HTTP request"), 50);
//  bResult = HttpSendRequest(hRequest, hdrs, (int)_tcslen(hdrs), 
//    (void*)szPOSTRequest, (DWORD)strlen(szPOSTRequest));
//    
//  if(bResult == FALSE)
//  {
//    m_Assync->SetProgress(_T("Error sending HTTP request"), 100, false);
//		goto exit; // Couldn't send request
//  }
//	  
//  m_Assync->SetProgress(_T("Sending error report over HTTP completed OK"), 10, true);
//    
//  HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE, szResponce, &dwBufSize, NULL); 
//  if(atoi(szResponce)!=200)
//  {
//    CString msg;
//    msg.Format(_T("Error! The server returned code %s"), CString(szResponce));
//    m_Assync->SetProgress(msg, 0);
//    goto exit;
//  }    
//
//  InternetReadFile(hRequest, szResponce, 1024, &dwBufSize);
//  szResponce[dwBufSize] = 0;
//  msg = CString(szResponce, dwBufSize);
//  msg = _T("Server returned:") + msg;
//  m_Assync->SetProgress(msg, 0);
//    
//  if(atoi(szResponce)!=200)
//  {
//    m_Assync->SetProgress(_T("Failed"), 100, false);
//    goto exit;
//  }
//
//  m_Assync->SetProgress(_T("Sent OK"), 100, false);
//  bStatus = TRUE;
//

  BOOL bStatus = FALSE;      // Resulting status
  strconv_t strconv;         // String conversion
  HINTERNET hSession = NULL; // Internet session
  HINTERNET hConnect = NULL; // Internet connection
  HINTERNET hRequest = NULL; // Handle to HTTP request
  TCHAR szProtocol[512];     // Protocol
  TCHAR szServer[512];       // Server name
  TCHAR szURI[1024];         // URI
  DWORD dwPort=0;            // Port

  // Create Internet session
  hSession = InternetOpen(_T("CrashRpt"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(hSession==NULL)
  {
    m_Assync->SetProgress(_T("Error opening Internet session"), 0);
	  goto cleanup; 
  }
  
  // Parse application-provided URL
  ParseURL(m_Request.m_sUrl, szProtocol, 512, szServer, 512, dwPort, szURI, 1024);

  // Connect to HTTP server
  m_Assync->SetProgress(_T("Connecting to server"), 5, false);

  hConnect = InternetConnect(hSession, szServer, (WORD)dwPort, 
    NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1);
	if(hConnect==NULL)
  {
    m_Assync->SetProgress(_T("Error connecting to server"), 0);
	  goto cleanup; 
  }

  if(m_Assync->IsCancelled()){ goto cleanup; }

  m_Assync->SetProgress(_T("Preparing HTTP request data"), 7, false);
  
cleanup:

  // Clean up
	if(hRequest) 
    InternetCloseHandle(hRequest);

	if(hConnect) 
    InternetCloseHandle(hConnect);

	if(hSession) 
    InternetCloseHandle(hSession);

  return bStatus;
}

BOOL CHttpRequestSender::CalcRequestSize(LONGLONG& lSize)
{
  lSize = 0;
  
  // Calculate summary size of all text fields included into request
  std::map<CString, CString>::iterator it;
  for(it=m_Request.m_aTextFields.begin(); it!=m_Request.m_aTextFields.end(); it++)
  {
    CString sPart;
    sPart.Format(_T("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\nContent-Transfer-Encoding:binary\r\n\r\n%s\r\n"),
      PART_BOUNDARY, it->first, it->second);
    lSize += sPart.GetLength();
  }

  // Calculate summary size of all files included into report
  std::map<CString, CHttpRequestFile>::iterator it2;
  for(it2=m_Request.m_aIncludedFiles.begin(); it2!=m_Request.m_aIncludedFiles.end(); it2++)
  {
    CString sPart;
    sPart.Format(_T("--%s\r\nContent-Disposition: attachment; filename=\"%s\"\r\nContent-Type:%s\r\nContent-Transfer-Encoding:binary\r\n\r\n\r\n"),
      PART_BOUNDARY, it2->first, it2->second.m_sSrcFileName, it2->second.m_sContentType);
    lSize += sPart.GetLength();
    
    // Calculate size of the attachment
    LARGE_INTEGER lFileSize;    
    HANDLE hFile = CreateFile(it2->second.m_sSrcFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if(hFile==INVALID_HANDLE_VALUE)
      return FALSE;
    BOOL bGetSize = GetFileSizeEx(hFile, &lFileSize);
    if(!bGetSize)
      return FALSE;

    lSize += (DWORD)lFileSize.QuadPart;
  }

  return TRUE;
}

BOOL CHttpRequestSender::FormatFormDataPart(CString sName, CString& sPart)
{
  std::map<CString, CString>::iterator it = m_Request.m_aTextFields.find(sName);
  if(it==m_Request.m_aTextFields.end())
    return FALSE;
 
  if(m_Request.m_bMultiPart)
  {
    sPart.Format(_T("--%s\r\nContent-Disposition: form-data; name=\"%s\"\r\nContent-Transfer-Encoding:binary\r\n\r\n%s\r\n"),
      PART_BOUNDARY, it->first, it->second);    
  }
  else
  {
    CBase64 base64;
    char*
    sPart.Format(_T("%s=%s"),
      base64.Encode(it->first.GetBuffer(0), it->fi it->second);    
  }
}

//BOOL CHttpRequestSender::SendMultiPart()
//{
//  BOOL bStatus = FALSE;
//  HINTERNET hSession = NULL;
//  HINTERNET hConnect = NULL;
//  
//  hSession = InternetOpen(_T("CrashRpt"), INTERNET_OPEN_TYPE_PRECONFIG,
//		NULL, NULL, 0);
//	if(!hSession)
//	{
//		goto cleanup;
//	}
//
//	hConnect = InternetConnect(hSession, argv[2], INTERNET_DEFAULT_HTTP_PORT,
//		NULL, NULL, INTERNET_SERVICE_HTTP,NULL, NULL);
//	if (!hConnect)
//		printf( "Failed to connect\n" );
//	else
//	{
//		HINTERNET hRequest = HttpOpenRequest(hConnect, "POST", argv[3], 
//			NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);
//		if (!hRequest)
//			printf( "Failed to open request handle\n" );
//		else
//		{
//			if(UseHttpSendReqEx(hRequest, dwPostSize))
//			{	
//				char pcBuffer[BUFFSIZE];
//				DWORD dwBytesRead;
//
//				printf("\nThe following was returned by the server:\n");
//				do
//				{	dwBytesRead=0;
//					if(InternetReadFile(hRequest, pcBuffer, BUFFSIZE-1, &dwBytesRead))
//					{
//						pcBuffer[dwBytesRead]=0x00; // Null-terminate buffer
//						printf("%s", pcBuffer);
//					}
//					else
//						printf("\nInternetReadFile failed");
//				}while(dwBytesRead>0);
//				printf("\n");
//			}
//			if (!InternetCloseHandle(hRequest))
//				printf( "Failed to close Request handle\n" );
//		}
//		if(!InternetCloseHandle(hConnect))
//			printf("Failed to close Connect handle\n");
//	}
//	if( InternetCloseHandle( hSession ) == FALSE )
//		printf( "Failed to close Session handle\n" );
//}
//
//BOOL CHttpRequestSender::UseHttpSendRequestEx()
//{
//  INTERNET_BUFFERS BufferIn;
//	DWORD dwBytesWritten;
//	int n;
//	BYTE pBuffer[1024];
//	BOOL bRet;
//
//	BufferIn.dwStructSize = sizeof( INTERNET_BUFFERS ); // Must be set or error will occur
//    BufferIn.Next = NULL; 
//    BufferIn.lpcszHeader = NULL;
//    BufferIn.dwHeadersLength = 0;
//    BufferIn.dwHeadersTotal = 0;
//    BufferIn.lpvBuffer = NULL;                
//    BufferIn.dwBufferLength = 0;
//    BufferIn.dwBufferTotal = dwPostSize; // This is the only member used other than dwStructSize
//    BufferIn.dwOffsetLow = 0;
//    BufferIn.dwOffsetHigh = 0;
//
//    if(!HttpSendRequestEx( hRequest, &BufferIn, NULL, 0, 0))
//    {
//        printf( "Error on HttpSendRequestEx %d\n",GetLastError() );
//        return FALSE;
//    }
//
//	FillMemory(pBuffer, 1024, 'D'); // Fill buffer with data
//
//	bRet=TRUE;
//	for(n=1; n<=(int)dwPostSize/1024 && bRet; n++)
//	{
//		if(bRet=InternetWriteFile( hRequest, pBuffer, 1024, &dwBytesWritten))
//			printf( "\r%d bytes sent.", n*1024);
//	}
//		
//	if(!bRet)
//	{
//        printf( "\nError on InternetWriteFile %lu\n",GetLastError() );
//        return FALSE;
//    }
//
//    if(!HttpEndRequest(hRequest, NULL, 0, 0))
//    {
//        printf( "Error on HttpEndRequest %lu \n", GetLastError());
//        return FALSE;
//    }
//
//	return TRUE;
//}

// Parses URL. This method's code was taken from 
// http://www.codeproject.com/KB/IP/simplehttpclient.aspx
void CHttpRequestSender::ParseURL(LPCTSTR szURL, LPTSTR szProtocol, UINT cbProtocol, 
  LPTSTR szAddress, UINT cbAddress, DWORD &dwPort, LPTSTR szURI, UINT cbURI)
{  
	cbURI;
	cbAddress;
	cbProtocol;
	
	DWORD dwPosition=0;
	BOOL bFlag=FALSE;

	while(_tcslen(szURL)>0 && dwPosition<_tcslen(szURL) && _tcsncmp((szURL+dwPosition), _T(":"), 1))
		++dwPosition;

	if(!_tcsncmp((szURL+dwPosition+1), _T("/"), 1)){	// is PROTOCOL
		if(szProtocol){
			_TCSNCPY_S(szProtocol, cbProtocol, szURL, dwPosition);
			szProtocol[dwPosition]=0;
		}
		bFlag=TRUE;
	}else{	// is HOST 
		if(szProtocol){
			_TCSNCPY_S(szProtocol, cbProtocol, _T("http"), 4);
			szProtocol[5]=0;
		}
	}

	DWORD dwStartPosition=0;
	
	if(bFlag){
		dwStartPosition=dwPosition+=3;				
	}else{
		dwStartPosition=dwPosition=0;
	}

	bFlag=FALSE;
	while(_tcslen(szURL)>0 && dwPosition<_tcslen(szURL) && _tcsncmp(szURL+dwPosition, _T("/"), 1))
			++dwPosition;

	DWORD dwFind=dwStartPosition;

	for(;dwFind<=dwPosition;dwFind++){
		if(!_tcsncmp((szURL+dwFind), _T(":"), 1)){ // find PORT
			bFlag=TRUE;
			break;
		}
	}

	if(bFlag)
  {
		TCHAR sztmp[256]=_T("");
		_TCSNCPY_S(sztmp, 256, (szURL+dwFind+1), dwPosition-dwFind);
		dwPort=_ttol(sztmp);
    int len = dwFind-dwStartPosition;
		_TCSNCPY_S(szAddress, cbAddress, (szURL+dwStartPosition), len);
    szAddress[len]=0;
	}
  else if(!_tcscmp(szProtocol,_T("https")))
  {
		dwPort=INTERNET_DEFAULT_HTTPS_PORT;
    int len = dwPosition-dwStartPosition;
		_TCSNCPY_S(szAddress, cbAddress, (szURL+dwStartPosition), len);
    szAddress[len]=0;
	}
  else 
  {
		dwPort=INTERNET_DEFAULT_HTTP_PORT;
    int len = dwPosition-dwStartPosition;
		_TCSNCPY_S(szAddress, cbAddress, (szURL+dwStartPosition), len);    
    szAddress[len]=0;
	}

	if(dwPosition<_tcslen(szURL))
  { 
    // find URI
    int len = (int)(_tcslen(szURL)-dwPosition);
		_TCSNCPY_S(szURI, cbURI, (szURL+dwPosition), len);
    szURI[len] = 0;
	}
  else
  {
		szURI[0]=0;
	}

	return;
}

