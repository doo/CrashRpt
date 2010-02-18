#pragma once
#include "stdafx.h"

class CURLEncode
{
private:
	static CString csUnsafeString;
	CString decToHex(TCHAR num, int radix);
	bool isUnsafe(TCHAR compareChar);
	CString convert(TCHAR val);

public:
	CURLEncode() { };
	virtual ~CURLEncode() { };
	CString URLEncode(CString sText);	
};

