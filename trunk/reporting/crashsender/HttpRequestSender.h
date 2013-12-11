/************************************************************************************* 
This file is a part of CrashRpt library.
Copyright (c) 2003-2013 The CrashRpt project authors. All Rights Reserved.

Use of this source code is governed by a BSD-style license
that can be found in the License.txt file in the root of the source
tree. All contributing project authors may
be found in the Authors.txt file in the root of the source tree.
***************************************************************************************/

// File: httpsend.h
// Description: Sends error report over HTTP connection.
// Authors: zexspectrum
// Date: 2009

#pragma once
#include "stdafx.h"
#include "AsyncNotification.h"


struct CHttpRequestFile
{  
    WTL::CString m_sSrcFileName;  // Name of the file attachment.
    WTL::CString m_sContentType;  // Content type.
};

// HTTP request information
class CHttpRequest
{
public:
    WTL::CString m_sUrl;      // Script URL  
    std::map<WTL::CString, std::string> m_aTextFields;    // Array of text fields to include into POST data
    std::map<WTL::CString, CHttpRequestFile> m_aIncludedFiles; // Array of binary files to include into POST data
};

// Sends HTTP request
// See also: RFC 1867 - Form-based File Upload in HTML (http://www.ietf.org/rfc/rfc1867.txt)
class CHttpRequestSender
{
public:

    CHttpRequestSender();

    // Sends HTTP request assynchroniously
    BOOL SendAssync(CHttpRequest& Request, AsyncNotification* an);

private:

    // Worker thread procedure
    static DWORD WINAPI WorkerThread(VOID* pParam);  

    BOOL InternalSend();

    // Used to calculate summary size of the request
    BOOL CalcRequestSize(LONGLONG& lSize);
    BOOL FormatTextPartHeader(WTL::CString sName, WTL::CString& sText);
    BOOL FormatTextPartFooter(WTL::CString sName, WTL::CString& sText);  
    BOOL FormatAttachmentPartHeader(WTL::CString sName, WTL::CString& sText);
    BOOL FormatAttachmentPartFooter(WTL::CString sName, WTL::CString& sText);
    BOOL FormatTrailingBoundary(WTL::CString& sBoundary);
    BOOL CalcTextPartSize(WTL::CString sFileName, LONGLONG& lSize);
    BOOL CalcAttachmentPartSize(WTL::CString sFileName, LONGLONG& lSize);
    BOOL WriteTextPart(HINTERNET hRequest, WTL::CString sName);
    BOOL WriteAttachmentPart(HINTERNET hRequest, WTL::CString sName);
    BOOL WriteTrailingBoundary(HINTERNET hRequest);
    void UploadProgress(DWORD dwBytesWritten);

    // This helper function is used to split URL into several parts
    void ParseURL(LPCTSTR szURL, LPTSTR szProtocol, UINT cbProtocol,
        LPTSTR szAddress, UINT cbAddress, DWORD &dwPort, LPTSTR szURI, UINT cbURI);

    CHttpRequest m_Request;       // HTTP request being sent
    AsyncNotification* m_async; // Used to communicate with the main thread  

    WTL::CString m_sFilePartHeaderFmt;
    WTL::CString m_sFilePartFooterFmt;
    WTL::CString m_sTextPartHeaderFmt;
    WTL::CString m_sTextPartFooterFmt;
    WTL::CString m_sBoundary;
    DWORD m_dwPostSize;
    DWORD m_dwUploaded;
};


