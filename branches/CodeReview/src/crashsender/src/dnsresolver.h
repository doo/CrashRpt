#pragma once
#include "dns.h"
#include <map>
#include <vector>

class _DNSR;
class CDnsResolver /*: protected CAsyncSocket*/
{
public:
	CDnsResolver();
	virtual ~CDnsResolver();

public:
	void SkipQuery(_DNSPACKET* pPacket);
	BOOL GetMX(LPCSTR Addr, std::vector<CString>& arr);
	BOOL Create(LPCSTR lpzDnsServer);

protected:
	typedef std::map<USHORT, _DNSR*>	dnsMap;
	dnsMap	m_DnsIndexMap;
	_DNSPACKET m_Dns;
	CString m_DnsServer;
	CComAutoCriticalSection m_LockDnsPacket;
protected:
	virtual void OnReceive(int nErrorCode);
	_DNSPACKET* Query(LPCSTR Addr, _DNSTYPE Type, _DNSCLASS Class = classIN);
	void Close();
	int SendTo(const void* lpBuf, int nBufLen, UINT nHostPort, LPCTSTR lpszHostAddress = NULL, int nFlags = 0); 
    int ReceiveFrom(void* lpBuf, int nBufLen, CString& rSocketAddress, UINT& rSocketPort, int nFlags = 0);

};
