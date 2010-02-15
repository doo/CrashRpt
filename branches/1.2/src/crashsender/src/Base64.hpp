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

///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2005-2007 Ernest Laurentin  (http://www.ernzo.com/)
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
// distribution.
///////////////////////////////////////////////////////////////////////////////
#ifndef BASE64_HPP
#define BASE64_HPP

typedef unsigned char uchar;
typedef unsigned int uint;

/**
 * CBase64
 * <pre>
 * Encode/Decode data according to Base64 encoding format
 * These routines use the 64-character subset of International
 * Alphabet IA5 discussed in RFC 1421 (printeable encoding) and RFC 1522
 * (base64 MIME).
 * 
 * 
 * Value Encoding  Value Encoding  Value Encoding  Value Encoding
 *     0 A            17 R            34 i            51 z
 *     1 B            18 S            35 j            52 0
 *     2 C            19 T            36 k            53 1
 *     3 D            20 U            37 l            54 2
 *     4 E            21 V            38 m            55 3
 *     5 F            22 W            39 n            56 4
 *     6 G            23 X            40 o            57 5
 *     7 H            24 Y            41 p            58 6
 *     8 I            25 Z            42 q            59 7
 *     9 J            26 a            43 r            60 8
 *    10 K            27 b            44 s            61 9
 *    11 L            28 c            45 t            62 +
 *    12 M            29 d            46 u            63 /
 *    13 N            30 e            47 v
 *    14 O            31 f            48 w         (pad) =
 *    15 P            32 g            49 x
 *    16 Q            33 h            50 y
 * </pre>
 */
class CBase64
{
public:

	CBase64() { }

	/**
	 * Convert to uchar
	 * @param c ASCII character
	 * @return unsigned character
	 */
	static uchar to_uchar (char c) { return c; }

	/**
	 * Calculate buffer size requirement
	 * @param length size of binary data in bytes
	 * @return size of required buffer including NULL
	 */
	size_t CalcBufferSize(size_t length) const { return ((length+2)/3)*4+(length/BASE64_NCHAR)+1; };

	/**
	 * Test if a character is a valid Base64 alphabet
	 * @param c character to test
	 * @return true if valid Base64 alphabet
	 */
	bool IsBase64Char(uchar c) const { return ( Decode( c ) != (uchar)-1 ); }

	/**
	 * Encode a byte to Base64 encoding
	 * @param c byte to convert to Base64 (6-bit)
	 * @return value byte in Base64 alphabet, or error (-1) for invalid input
	 */
	uchar Encode(uchar c) const { return ( c < 64 ? base64_encode[ c ] : (uchar)-1 ); }

	/**
	 * Encode an array of bytes to Base64 string
	 * @param in byte pointer of data source
	 * @param length size of data source
	 * @param outbuf byte pointer of destination buffer
	 * @param size size of destination buffer
	 * @param option newline option
	 * @return number of bytes copied, or error (-1) for invalid input
	 */
	size_t Encode(const uchar* in, size_t length, uchar* outbuf, size_t size, uint option = BASE64_STANDARD) const;

	/**
	 * Decode a Base64 alphabet data to 6-bit representation
	 * @param c Alphabet data to convert to binary value
	 * @return binary value (0-63), or error (-1) for invalid input
	 */
	uchar Decode(uchar c) const;

	/**
	 * Decode an array of bytes to binary.
	 * It's safe for 'in' and 'outbuf' to be the same buffer.
	 * @param in byte pointer of data source (Base64 Alphabet)
	 * @param length size of data source
	 * @param outbuf byte pointer of destination buffer
	 * @param size size of destination buffer
	 * @param option newline option
	 * @return number of bytes copied, or error (-1) for invalid input
	 */
	size_t Decode(const uchar* in, size_t length, uchar* outbuf, size_t size, uint option = BASE64_STANDARD) const;

private:
	enum {
		BASE64_PAD      = '=',	///< Pad character
		BASE64_CR       = '\r',	///< Carriage return
		BASE64_LF       = '\n',	///< New line separator
		BASE64_NCHAR    = 76,	///< Number of characters per line when encoding
		BASE64_STANDARD = 0x01	///< Multi line support
	};
	static uchar base64_encode[];	///< Base64 Alphabet encoder-string
};

__declspec(selectany) uchar CBase64::base64_encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline uchar CBase64::Decode(uchar c) const
{
	if (c == '/')
		return 63;
	if (c == '+')
		return 62;
	if (c >= '0' && c <= '9')
		return c - '0' + 52;
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	return (uchar)-1;
}

inline size_t CBase64::Encode(const uchar* in, size_t length, uchar* outbuf, size_t size, uint option /*=BASE64_STANDARD*/) const
{
	size_t cnt = 0;
	size_t rc  = 0;
	while ( rc < length )
	{
		if ( size < 4 )
		{
			// insufficient buffer
			cnt = (size_t)-1;
			break;
		}
		size_t left = length - rc;
		switch( left )
		{
		case 1:
			*outbuf++ = Encode((in[rc] >> 2));
			*outbuf++ = Encode((in[rc] << 4) & 0x30);
			*outbuf++ = to_uchar(BASE64_PAD);
			*outbuf++ = to_uchar(BASE64_PAD);
			break;
		case 2:
			*outbuf++ = Encode((in[rc] >> 2));
			*outbuf++ = Encode(((in[rc] << 4) & 0x30) | ((in[rc + 1] >> 4) & 0x0f));
			*outbuf++ = Encode((in[rc+1] << 2) & 0x3c);
			*outbuf++ = to_uchar(BASE64_PAD);
			break;
		default:
			*outbuf++ = Encode((in[rc] >> 2));
			*outbuf++ = Encode(((in[rc] << 4) & 0x30) | ((in[rc + 1] >> 4) & 0x0f));
			*outbuf++ = Encode(((in[rc + 1] << 2) & 0x3c) | ((in[rc + 2] >> 6)));
			*outbuf++ = Encode((in[rc + 2]) & 0x3f);
			break;
		}
		rc  += 3;
		cnt += 4;
		size -= 4;
		if ( (option&BASE64_STANDARD) && !(cnt%BASE64_NCHAR))
			*outbuf++ = to_uchar(BASE64_LF);
	}

	if ( cnt != -1 )
		*outbuf = 0;
	return cnt;
}

inline size_t CBase64::Decode(const uchar* in, size_t length, uchar* outbuf, size_t size, uint /*option =BASE64_STANDARD*/) const
{
	size_t cnt = 0;
	size_t rc  = 0;
	while ( (*in) && (rc < length) )
	{
		if ( (in[rc] == BASE64_CR || in[rc] == BASE64_LF))
			rc++;
		else if ( size > 1 )
		{
			uchar b1 = ( Decode(in[rc])<<2 &0xfc ) | ( Decode(in[rc+1])>>4 &0x03 );
			*outbuf++ = b1;
			--size;
			++cnt;

			if ((size>1) && in[rc + 2] != BASE64_PAD)
			{
				uchar b2 = ( Decode(in[rc+1])<<4 &0xf0 ) | ( Decode(in[rc+2])>>2 &0x0f );
				*outbuf++ = b2;
				--size;
				++cnt;
			}
			if ((size>1) && in[rc + 3] != BASE64_PAD)
			{
				uchar b3 = ( Decode(in[rc+2])<<6 &0xc0 ) | ( Decode(in[rc+3] )& 0x3f );
				*outbuf++ = b3;
				--size;
				++cnt;
			}
			rc += 4;
		}
		else
		{
			// insufficient buffer
			cnt = (size_t)-1;
			break;
		}
	}
	if ( cnt != -1 )
		*outbuf = 0;
	return cnt;
}

#endif //BASE64_HPP