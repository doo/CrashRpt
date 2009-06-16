#include "stdafx.h"
#include "httpsend.h"
#include <wininet.h>
#include <atlenc.h>

BOOL CHttpSender::Send(CString sURL, CString sFileName)
{ 
  BOOL bStatus = FALSE;
	TCHAR* hdrs = _T("Content-Type: application/x-www-form-urlencoded");
	LPCTSTR accept[2]={_T("*/*"), NULL};
  size_t uFileSize = 0;
  BYTE* uchFileData = NULL;
  int nEncodedFileDataLen = 0;  
  HINTERNET hSession = NULL;
  HINTERNET hConnect = NULL;
  HINTERNET hRequest = NULL;
  TCHAR szProtocol[512];
  TCHAR szServer[512];
  TCHAR szURI[1024];
  DWORD dwPort;
  struct _stat st;
  int res = -1;
  FILE* f = NULL;
  DWORD dwFlags = 0;
  BOOL bEncoded = FALSE;
  BOOL bResult = FALSE;
  char* chPOSTRequest = NULL;
  CStringA sPOSTRequest;
  char* szPrefix="crashrpt=\"";
  char* szSuffix="\"";

  // Create Internet session
	hSession = InternetOpen(_T("CrashRpt"),
		INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(hSession==NULL)
	  goto exit; // Couldn't create internet session
  
  ParseURL(sURL, szProtocol, 512, szServer, 512, dwPort, szURI, 1024);

  // Connect to server
	hConnect = InternetConnect(
    hSession, 
    szServer,
		INTERNET_DEFAULT_HTTP_PORT, 
    NULL, 
    NULL, 
    INTERNET_SERVICE_HTTP, 
    0, 
    1);
	
	if(hConnect==NULL)
	  goto exit; // Couldn't connect
	
  // Load file data into memory
  res = _tstat(sFileName.GetBuffer(), &st);
  if(res!=0)
    goto exit; // File not found
  
  uFileSize = st.st_size;
  uchFileData = new BYTE[uFileSize];
  _tfopen_s(&f, sFileName.GetBuffer(), _T("rb"));
  if(!f || fread(uchFileData, uFileSize, 1, f)!=1)
  {
    goto exit;  
  }
  fclose(f);

  // Encode file data using BASE64
  dwFlags = ATL_BASE64_FLAG_NONE;
  nEncodedFileDataLen = Base64EncodeGetRequiredLength(uFileSize, dwFlags);
  int nPOSTRequestLen = nEncodedFileDataLen+strlen(szPrefix)+strlen(szSuffix);

  chPOSTRequest = new char[nPOSTRequestLen];    
  memset(chPOSTRequest, 0, nPOSTRequestLen);

  memcpy(chPOSTRequest, szPrefix, strlen(szPrefix));  
  
  bEncoded = Base64Encode(uchFileData, uFileSize, chPOSTRequest+strlen(szPrefix), 
                          &nEncodedFileDataLen, dwFlags);
  if(!bEncoded)
    goto exit;
  
  strcpy_s(chPOSTRequest+strlen(szPrefix)+nEncodedFileDataLen, nPOSTRequestLen, szSuffix);

  sPOSTRequest = CStringA(chPOSTRequest, nPOSTRequestLen);
  sPOSTRequest.Replace("+", "%2B");
  sPOSTRequest.Replace("/", "%2F");

  // Send POST request
  hRequest = HttpOpenRequest(hConnect, _T("POST"),
		                         szURI, NULL, NULL, accept, 0, 1);	
	if(hRequest==NULL)
	  return FALSE; // Coudn't open request	

  bResult = HttpSendRequest(hRequest, hdrs, _tcslen(hdrs), 
    sPOSTRequest.GetBuffer(), sPOSTRequest.GetLength());
    
  if(bResult == FALSE)
		return FALSE; // Couldn't send request
	  
  bStatus = TRUE;

exit:

  // Clean up
	if(hRequest) 
    InternetCloseHandle(hRequest);

	if(hConnect) 
    InternetCloseHandle(hConnect);

	if(hSession) 
    InternetCloseHandle(hSession);

  if(chPOSTRequest)
    delete [] chPOSTRequest;
    
  if(uchFileData)
    delete [] uchFileData;

  if(f)
    fclose(f);

  return bStatus;
}


// This method's code was taken from 
// http://www.codeproject.com/KB/IP/simplehttpclient.aspx
void CHttpSender::ParseURL(LPCTSTR szURL, LPTSTR szProtocol, UINT cbProtocol, 
                           LPTSTR szAddress, UINT cbAddress, DWORD &dwPort, 
                           LPTSTR szURI, UINT cbURI)
{  
	TCHAR szPort[256]=_T("");
	DWORD dwPosition=0;
	BOOL bFlag=FALSE;

	while(_tcslen(szURL)>0 && dwPosition<_tcslen(szURL) && _tcsncmp((szURL+dwPosition), _T(":"), 1))
		++dwPosition;

	if(!_tcsncmp((szURL+dwPosition+1), _T("/"), 1)){	// is PROTOCOL
		if(szProtocol){
			_tcsncpy_s(szProtocol, cbProtocol, szURL, dwPosition);
			szProtocol[dwPosition]=0;
		}
		bFlag=TRUE;
	}else{	// is HOST 
		if(szProtocol){
			_tcsncpy_s(szProtocol, cbProtocol, _T("http"), 4);
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
		_tcsncpy_s(sztmp, 256, (szURL+dwFind+1), dwPosition-dwFind);
		dwPort=_ttol(sztmp);
    int len = dwFind-dwStartPosition;
		_tcsncpy_s(szAddress, cbAddress, (szURL+dwStartPosition), len);
    szAddress[len]=0;
	}
  else if(!_tcscmp(szProtocol,_T("https")))
  {
		dwPort=INTERNET_DEFAULT_HTTPS_PORT;
    int len = dwPosition-dwStartPosition;
		_tcsncpy_s(szAddress, cbAddress, (szURL+dwStartPosition), len);
    szAddress[len]=0;
	}
  else 
  {
		dwPort=INTERNET_DEFAULT_HTTP_PORT;
    int len = dwPosition-dwStartPosition;
		_tcsncpy_s(szAddress, cbAddress, (szURL+dwStartPosition), len);    
    szAddress[len]=0;
	}

	if(dwPosition<_tcslen(szURL))
  { 
    // find URI
    int len = _tcslen(szURL)-dwPosition;
		_tcsncpy_s(szURI, cbURI, (szURL+dwPosition), len);
    szURI[len] = 0;
	}
  else
  {
		szURI[0]=0;
	}

	return;
}

