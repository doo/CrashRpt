#ifndef _DNS
#define _DNS

#include "stdafx.h"

enum _DNSCLASS{
	classIN = 1,
		classCS,
		classCH,
		classHS,
		classANY=255
};

enum _DNSTYPE{
	typeA = 1,
		typeNS,
		typeMD,
		typeMF,
		typeCNAME,
		typeSOA,
		typeMB,
		typeMG,
		typeMR,
		typeNULL,
		typeWKS,
		typePTR,
		typeHINFO,
		typeMINFO,
		typeMX,
		typeTXT,
		typeAXFR = 252,
		typeMAILB,
		typeMAILA,
		typeAll
};

struct _DNSHEADER{
	USHORT	id;
	
	USHORT	RD		: 1;
	USHORT	TC		: 1;
	USHORT	AA		: 1;
	USHORT	Opcode	: 4;
	USHORT	QR		: 1;
	
	USHORT	RCODE	: 4;
	USHORT	Z		: 3;
	USHORT	RA		: 1;
	
	USHORT	QDCOUNT;
	USHORT	ANCOUNT;
	USHORT	NSCOUNT;
	USHORT	ARCOUNT;
public:
	_DNSHEADER()
	{
		memset(this, 0, sizeof(this));
	}
	void Normalize()
	{
		QDCOUNT = htons(QDCOUNT);
		ANCOUNT = htons(ANCOUNT);
		NSCOUNT = htons(NSCOUNT);
		ARCOUNT = htons(ARCOUNT);
	}
};

struct _DNSPACKET{
public:
	_DNSHEADER	Header;
private:
	char		pvtData[512-sizeof(_DNSHEADER)];
public:
	USHORT Offset;
	
	int Empty()
	{
		USHORT id = ++Header.id;
		memset(this, 0, GetSize());
		Header.id = id;
		
		Header.RD	= 1;
		Offset		= sizeof(Header);
		return id;
	};
	
	_DNSPACKET()	{Header.id	= 0;Empty();};
	void Normalize(){Header.Normalize();}
	
	inline operator BYTE*()			{return (BYTE*)this;};
	inline USHORT	GetSize()const	{return 512;};
};

struct _QNAME
{
	char	Data[512];
public:
	USHORT		Offset;
	
	void Empty(){
		memset(Data, 0, sizeof(Data));
		Offset = 0;
	}
	
	_QNAME(){Empty();};
	
	inline USHORT	GetSize()const{return Offset+1;};
	
	int	Add(LPCSTR str)
	{
		USHORT nLen		= 0;
		char* pCurr		= Data+Offset;
		char* pLast		= pCurr;
		
		while(*str)
		{
			if (*str == '.'){
				*pLast = (BYTE)nLen;
				
				pLast = ++pCurr;
				Offset += nLen + 1;
				str++;
				nLen = 0;
			}else{
				*++pCurr = *str++;
				nLen++;
			}
		}
		if (nLen){
			*pLast = (BYTE)nLen;
			Offset += nLen+1;
		}
		return TRUE;
	}
	
	char* Get(int& nLen)
	{
		char* pCurr	= Data;//+Offset;
		int len;
		nLen	= 1;
		
		while(len = *pCurr++){
			nLen	+= len+1;
			pCurr	+= len;
		}
		
		char* pBuff = (char*)malloc(nLen);
		char* pTmp = pBuff;
		
		pCurr = Data;
		while((len = *pCurr++) || *pCurr)
		{
			memcpy(pTmp, pCurr, len);
			pTmp	+= len;
			pCurr	+= len;
			if (*pCurr)
				*pTmp++ = '.';
		}
		*pTmp = 0;
		return pBuff;
	}
	
	BOOL Add(_DNSPACKET& dns, USHORT& nOffset)
	{
		memcpy((BYTE*)dns+nOffset, this, GetSize());
		
		nOffset += GetSize();
		return TRUE;
	}
	
	BOOL Fill(_DNSPACKET& dns, USHORT& nOffset)
	{
		BYTE* pCurr	= (BYTE*)dns+nOffset;
		int len;
		
		BOOL bIsCompressed = FALSE;
		
		while(len = *pCurr++)
		{
			if ((len & 0xc0) == 0xc0)
			{
				if (!bIsCompressed)
				{
					nOffset++;
					bIsCompressed = TRUE;
				}
				
				pCurr = (BYTE*)dns + (BYTE)*(BYTE*)pCurr;
			}else{
				memcpy(Data+Offset, pCurr-1, len + 1);
				Offset += len + 1;
				
				pCurr	+= len;
				
				if (!bIsCompressed)
					nOffset	+= len + 1;
			}
		}
		nOffset++;
		return TRUE;
	}
	
	};
	
	struct _DNSA
	{
		long	addr;
		_DNSA()
		{
			addr = 0;
		}
		
		void Normalize()
		{
			addr = htonl(addr);
		}
		
		BOOL Add(_DNSPACKET& dns, USHORT& nOffset)
		{
			memcpy(dns+nOffset, &addr, sizeof(addr));
			nOffset += sizeof(addr);
			return TRUE;
		}
		
		BOOL Fill(_DNSPACKET& dns, USHORT& nOffset)
		{
			memcpy(&addr, dns+nOffset, sizeof(addr));
			nOffset += sizeof(addr);
			return TRUE;
		}
	};
	
	struct _DNSNS
	{
		_QNAME	Server;
		
	public:
		inline void Empty()
		{
			Server.Empty();
		}
		
		_DNSNS(){
			Empty();
		}
		
		inline BOOL Fill(_DNSPACKET& dns, USHORT& nOffset)
		{
			return Server.Fill(dns, nOffset);
		}
		
		inline BOOL Add(_DNSPACKET& dns, USHORT& nOffset)
		{
			return Server.Add(dns, nOffset);
		}
	};
	
	struct _DNSMX
	{
		USHORT	Reference;
		_QNAME	Exchange;
		
	public:
		void Empty()
		{
			Reference = 0;
			Exchange.Empty();
		}
		
		_DNSMX(){
			Empty();
		}
		
		void Normalize()
		{
			Reference	= htons(Reference);
		}
		
		BOOL Fill(_DNSPACKET& dns, USHORT& nOffset)
		{
			memcpy(&Reference, dns+nOffset, sizeof(Reference));
			nOffset += sizeof(Reference);
			
			Exchange.Fill(dns, nOffset);
			return TRUE;
		}
		
		BOOL Add(_DNSPACKET& dns, USHORT& nOffset)
		{
			memcpy(dns+nOffset, &Reference, sizeof(Reference));
			nOffset += sizeof(Reference);
			
			Exchange.Add(dns, nOffset);
			return TRUE;
		}
	};
	
	struct _DNSSOA{
		_QNAME	mName;
		_QNAME	rName;
		long	Serial;
		long	Refresh;
		long	Retry;
		long	Expire;
		long	Minimum;
		
	public:
		void Empty()
		{
			mName.Empty();
			rName.Empty();
			memset(&Serial, 0, 5*4);
		}
		
		_DNSSOA(){
			Empty();
		}
		
		void Normalize()
		{
			Serial	= htonl(Serial);
			Refresh = htonl(Refresh);
			Retry	= htonl(Retry);
			Expire	= htonl(Expire);
			Minimum	= htonl(Minimum);
		}
		
		BOOL Fill(_DNSPACKET& dns, USHORT& nOffset)
		{
			mName.Fill(dns, nOffset);
			rName.Fill(dns, nOffset);
			
			memcpy(&Serial, (BYTE*)dns + nOffset, 5*4);
			nOffset	+= 5*4;
			return TRUE;
		}
		
		BOOL Add(_DNSPACKET& dns, USHORT& nOffset)
		{
			mName.Add(dns, nOffset);
			rName.Add(dns, nOffset);
			
			memcpy((BYTE*)dns+nOffset, &Serial, 5*4);
			nOffset += 5*4;
			return TRUE;
		}
	};
	
	struct _DNSQUESTION
	{
	public:
		_QNAME		qName;
		_DNSTYPE	Type;
		_DNSCLASS	Class;
	public:
		void Empty()
		{
			qName.Empty();
			Type	= typeAll;
			Class	= classANY;
		}
		
		_DNSQUESTION()
		{
			Empty();
		}
		
		void Normalize()
		{
			Type = (_DNSTYPE)htons(Type);
			Class = (_DNSCLASS)htons(Class);
		}
		
		int Add(_DNSPACKET& dns, USHORT& nOffset)
		{
			dns.Header.QDCOUNT++;
			qName.Add(dns, nOffset);
			
			*(USHORT*)((BYTE*)dns+nOffset) = Type;
			nOffset += 2;
			*(USHORT*)((BYTE*)dns+nOffset) = Class;
			nOffset += 2;
			return TRUE;
		}
		
		BOOL Fill(_DNSPACKET& dns, USHORT& nOffset)
		{
			if (!qName.Fill(dns, nOffset))
				return FALSE;
			
			Type = *(_DNSTYPE*)((BYTE*)dns+nOffset);
			nOffset += 2;
			
			Class = *(_DNSCLASS*)((BYTE*)dns+nOffset);
			nOffset += 2;
			
			return TRUE;
		}
		inline USHORT GetSize()const{return qName.GetSize() + 4;};
	};
	
	struct _DNSRR{
		_DNSQUESTION	q;
		long			ttl;
		USHORT			rDataLen;
		BYTE*			rData;
	public:
		_DNSPACKET*		pDns;
		USHORT			Offset;
	public:
		void Empty()
		{
			q.Empty();
			if (rData)
				free(rData);
			
			rDataLen	= 0;
			rData		= NULL;
			ttl			= 0;
			Offset		= 0;
		}
		
		_DNSRR(_DNSPACKET& rDns)
		{
			pDns		= &rDns;
			rData		= NULL;
			Empty();
		}
		~_DNSRR(){
			Empty();
		}
		
		void Normalize()
		{
			q.Normalize();
			ttl = htonl(ttl);
			rDataLen = htons(rDataLen);
		}
		
		int Add(_DNSPACKET& dns, USHORT& nOffset)
		{
			q.Add(dns, nOffset);
			
			memcpy((BYTE*)dns+nOffset, &ttl, sizeof(ttl));
			nOffset += sizeof(ttl);
			
			if (rDataLen)
			{
				memcpy((BYTE*)dns+nOffset, &rDataLen, sizeof(rDataLen));;
				nOffset	+= sizeof(rDataLen);
				
				int len = htonl(rDataLen);
				memcpy((BYTE*)dns+nOffset, rData, len);
				nOffset += len;
			}
			return TRUE;
		}
		
		BOOL Fill(_DNSPACKET& dns, USHORT& nOffset)
		{
			ATLASSERT(rData == NULL);
			
			if (!q.Fill(dns, nOffset))
				return FALSE;
			
			char* pCurr = (char*)(BYTE*)dns+nOffset;
			ttl = *(long*)pCurr;
			nOffset	+= 4;
			pCurr	+= 4;
			
			rDataLen = *(USHORT*)pCurr;
			nOffset	+= 2;
			pCurr	+= 2;
			
			if (rDataLen)
			{
				Offset = nOffset;
				int len = htons(rDataLen);
				rData = (BYTE*)malloc(len);
				memcpy(rData, pCurr, len);
				nOffset += len;
			}
			return TRUE;
		}
		inline USHORT GetSize()const{return rDataLen;};
	};
	
#endif