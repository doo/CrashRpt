#include <stdafx.h>
#include "UrlEncode.h"
#include <math.h>

// HEX Values array
char hexVals[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
// UNSAFE String
CString CURLEncode::csUnsafeString= "\"<>%\\^[]`+$,@:;/!#?=&";

// PURPOSE OF THIS FUNCTION IS TO CONVERT A GIVEN CHAR TO URL HEX FORM
CString CURLEncode::convert(TCHAR val) 
{
	CString csRet;
	csRet += _T("%");
	csRet += decToHex(val, 16);	
	return  csRet;
}

// THIS IS A HELPER FUNCTION.
// PURPOSE OF THIS FUNCTION IS TO GENERATE A HEX REPRESENTATION OF GIVEN CHARACTER
CString CURLEncode::decToHex(TCHAR num, int radix)
{	
	int temp=0;	
	CString csTmp;
	int num_char;
	num_char = (int) num;
	
	// ISO-8859-1 
	// IF THE IF LOOP IS COMMENTED, THE CODE WILL FAIL TO GENERATE A 
	// PROPER URL ENCODE FOR THE CHARACTERS WHOSE RANGE IN 127-255(DECIMAL)
	if (num_char < 0)
		num_char = 256 + num_char;

	while (num_char >= radix)
    {
		temp = num_char % radix;
		num_char = (int)floor((double)num_char / radix);
		csTmp = hexVals[temp];
    }
	
	csTmp += hexVals[num_char];

	if(csTmp.GetLength() < 2)
	{
		csTmp += '0';
	}

	CString strdecToHex(csTmp);
	// Reverse the String
	strdecToHex.MakeReverse(); 
	
	return strdecToHex;
}

// PURPOSE OF THIS FUNCTION IS TO CHECK TO SEE IF A CHAR IS URL UNSAFE.
// TRUE = UNSAFE, FALSE = SAFE
bool CURLEncode::isUnsafe(TCHAR compareChar)
{
	bool bcharfound = false;
	TCHAR tmpsafeChar;
	int m_strLen = 0;
	
	m_strLen = csUnsafeString.GetLength();
	for(int ichar_pos = 0; ichar_pos < m_strLen ;ichar_pos++)
	{
		tmpsafeChar = csUnsafeString.GetAt(ichar_pos); 
		if(tmpsafeChar == compareChar)
		{ 
			bcharfound = true;
			break;
		} 
	}
	int char_ascii_value = 0;
	//char_ascii_value = __toascii(compareChar);
	char_ascii_value = (int) compareChar;

	if(bcharfound == false &&  char_ascii_value > 32 && char_ascii_value < 123)
	{
		return false;
	}
	// found no unsafe chars, return false		
	else
	{
		return true;
	}
	
	return true;
}

// PURPOSE OF THIS FUNCTION IS TO CONVERT A STRING 
// TO URL ENCODE FORM.
CString CURLEncode::URLEncode(CString cText)
{	
	int pos = -1;
	CString cEncodedText;	
	int nLength = -1;
	
	nLength = cText.GetLength();
	
	for(pos = 0; pos < nLength; pos++)
	{
		TCHAR ch = cText.GetAt(pos);
		if(!isUnsafe(ch))
		{
			// Safe Character				
			cEncodedText += CString((char)ch);
		}
		else
		{
			// get Hex Value of the Character
			cEncodedText += convert(ch);
		}
	}
	
	return cEncodedText;
}

