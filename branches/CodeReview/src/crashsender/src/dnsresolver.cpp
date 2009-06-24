// DnsResolver.cpp : implementation file
//

//#include "stdafx.h"
#include "dns.h"
#include "dnsresolver.h"
#include <time.h>

/////////////////////////////////////////////////////////////////////////////
// CDnsResolver

CDnsResolver::CDnsResolver()
{
}

CDnsResolver::~CDnsResolver()
{
	Close();
}


/////////////////////////////////////////////////////////////////////////////
// CDnsResolver member functions
class _DNSR
{
public:
	BOOL			bIsUnlock;
	_DNSPACKET*		pPacket;
	_DNSR(){pPacket=NULL;bIsUnlock = FALSE;};
};

_DNSPACKET* CDnsResolver::Query(LPCSTR Addr, _DNSTYPE Type, _DNSCLASS Class)
{
	m_LockDnsPacket.Lock();

	m_Dns.Empty();		//	id++;
	_DNSRR rr(m_Dns);	//	Resource

	rr.q.qName.Add(Addr);	//	Query host name
	rr.q.Type = Type;		//	Query type
	rr.q.Class = Class;		//	Query class

	rr.Normalize();				//	Convert to UNIX byte order
	rr.Add(m_Dns, m_Dns.Offset);//	AddTo DNS Resource quering

	m_Dns.Normalize();		//	Convert to UNIX byte order

	_DNSR DnsR;

	USHORT nID = m_Dns.Header.id;	//	Create Callback ID
	m_DnsIndexMap[nID] = &DnsR;		//	Set to MAP

	SendTo(&m_Dns, m_Dns.GetSize(), 53, m_DnsServer);	//	Send query To DNS Server

	m_LockDnsPacket.Unlock();	//	Ulock DNS Resource

	MSG msg;
	//CTime time(CTime::GetCurrentTime());
	do	//	Wait answer with rr identify // may user WaitForSingleObject but this stopping app's messaging
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}else
			Sleep(10);	//	Don't use CPU time

	//	While not timeout or answer received
	}while(!DnsR.bIsUnlock /*( && (CTime::GetCurrentTime() - time).GetTotalSeconds() < 2)*/);

	m_DnsIndexMap.erase(nID);
	
	if (DnsR.pPacket == NULL ||	//	If don't have errors
		DnsR.pPacket->Header.RCODE == 0) return DnsR.pPacket;

	delete DnsR.pPacket;
	return NULL;
}

BOOL CDnsResolver::Create(LPCSTR lpzDnsServer)
{
	m_DnsServer = lpzDnsServer;
	//return CAsyncSocket::Create(0, SOCK_DGRAM);
	return TRUE;
}

//	Can call from threads with safe using
BOOL CDnsResolver::GetMX(LPCSTR Addr, std::vector<CString> &arr)
{
	BOOL nRet = FALSE;
	for(int i=0; i < 3; i++)	//	i - retry count
	{
		_DNSPACKET* pPacket = Query(Addr, typeMX);
		if (pPacket == NULL) continue;	//	Answer don't received

		pPacket->Normalize();	//	Convert byte ordering
		
		SkipQuery(pPacket);		//	Skip some query data

		while(pPacket->Header.ANCOUNT--)	//	Look answers
		{
			_DNSRR rr(*pPacket);	//	Resource data
			if (!rr.Fill(*pPacket, pPacket->Offset))	//	Can't fill packet
				break;

			rr.Normalize();	//	Convert byte ordering

			_DNSMX mx;		//	Mail exchange packet
			if (!mx.Fill(*pPacket, rr.Offset))	//	Can't fill packet
				break;

			mx.Normalize();	//	Convert byte ordering

			int nLen;
			char* pBuff = mx.Exchange.Get(nLen);	//	Decode answer string
			arr.push_back(pBuff);	//	Add to array
			free(pBuff);

			nRet = TRUE;	//	Answer received
		}
		delete pPacket;
		break;
	}
	return nRet;
}

void CDnsResolver::SkipQuery(_DNSPACKET* pPacket)
{
	ATLASSERT(pPacket);

	while(pPacket->Header.QDCOUNT--)	//	Skip questions
	{
		_DNSQUESTION query;
		query.Fill(*pPacket, pPacket->Offset);
	}
}

void CDnsResolver::OnReceive(int nErrorCode)
{
	static CString	m_Temp;
	static UINT		m_TempPort;

	_DNSPACKET* pPacket = new _DNSPACKET;

	int nLen = ReceiveFrom(pPacket, pPacket->GetSize(), m_Temp, m_TempPort);

	dnsMap::iterator it = m_DnsIndexMap.find(pPacket->Header.id);
	if (it != m_DnsIndexMap.end())
	{
		_DNSR* pDnsR = it->second;

		ATLASSERT(pDnsR->pPacket == NULL);
		pDnsR->pPacket = pPacket;
		pDnsR->bIsUnlock = TRUE;		
	}else{
		delete pPacket;
	}
}

void CDnsResolver::Close()
{
}

int CDnsResolver::SendTo(const void* lpBuf, int nBufLen, UINT nHostPort, LPCTSTR lpszHostAddress, int nFlags)
{
	return 0;
}

int CDnsResolver::ReceiveFrom(void* lpBuf, int nBufLen, CString& rSocketAddress, UINT& rSocketPort, int nFlags)
{
	return 0;
}



