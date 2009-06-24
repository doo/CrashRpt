#include "stdafx.h"
#include "httpsend.h"
#include <wininet.h>
#include <sys/stat.h>
#include "base64.h"
#include <string>

void _str_replace(std::string& str, char* charToReplace, char* strToInsert)
{  
  size_t found;

  size_t len = strlen(strToInsert);

  found=str.find_first_of(charToReplace);
  while (found!=std::string::npos)
  {
    str.replace(found, 1, strToInsert);
    found=str.find_first_of(charToReplace, found+len);
  }  
}

BOOL CHttpSender::Send(CString sURL, CString sFileName)
{ 
  BOOL bStatus = FALSE;
	TCHAR* hdrs = _T("Content-Type: application/x-www-form-urlencoded");
	LPCTSTR accept[2]={_T("*/*"), NULL};
  int uFileSize = 0;
  BYTE* uchFileData = NULL;
  //int nEncodedFileDataLen = 0;  
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
  //DWORD dwFlags = 0;
  //BOOL bEncoded = FALSE;
  BOOL bResult = FALSE;
  char* chPOSTRequest = NULL;
  std::string sPOSTRequest;
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
  res = _tstat(sFileName.GetBuffer(0), &st);
  if(res!=0)
    goto exit; // File not found
  
  uFileSize = st.st_size;
  uchFileData = new BYTE[uFileSize];
#if _MSC_VER<1400
  f = _tfopen(sFileName.GetBuffer(0), _T("rb"));
#else
  _tfopen_s(&f, sFileName.GetBuffer(0), _T("rb"));
#endif
  if(!f || fread(uchFileData, uFileSize, 1, f)!=1)
  {
    goto exit;  
  }
  fclose(f);

  sPOSTRequest = base64_encode(uchFileData, uFileSize);
  sPOSTRequest = szPrefix + sPOSTRequest + szSuffix;  
  _str_replace(sPOSTRequest, "+", "%2B");
  _str_replace(sPOSTRequest, "/", "%2F");
  
  // Send POST request
  hRequest = HttpOpenRequest(hConnect, _T("POST"),
		                         szURI, NULL, NULL, accept, 0, 1);	
	if(hRequest==NULL)
	  return FALSE; // Coudn't open request	

  bResult = HttpSendRequest(hRequest, hdrs, (int)_tcslen(hdrs), 
    (void*)sPOSTRequest.c_str(), (DWORD)sPOSTRequest.length());
    
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
	cbURI;
	cbAddress;
	cbProtocol;

	//TCHAR szPort[256]=_T("");
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

