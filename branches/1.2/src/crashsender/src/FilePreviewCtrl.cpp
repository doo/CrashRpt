#include "stdafx.h"
#include "FilePreviewCtrl.h"

//-----------------------------------------------------------------------------
// CFileMemoryMapping implementation
//-----------------------------------------------------------------------------

CFileMemoryMapping::CFileMemoryMapping()  
{
  m_hFile = INVALID_HANDLE_VALUE;
  m_uFileLength = 0;
  m_hFileMapping = NULL;
  m_pViewStartPtr = NULL;
  
  SYSTEM_INFO si;  
  GetSystemInfo(&si);
  m_dwAllocGranularity = si.dwAllocationGranularity;
}

CFileMemoryMapping::~CFileMemoryMapping()
{
  Destroy();
}


BOOL CFileMemoryMapping::Init(LPCTSTR szFileName)
{
  if(m_hFile!=INVALID_HANDLE_VALUE)
  {
    Destroy();    
  }

	m_hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(m_hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	
	m_hFileMapping = CreateFileMapping(m_hFile, 0, PAGE_READONLY, 0, 0, 0);
  LARGE_INTEGER size;
	GetFileSizeEx(m_hFile, &size);	
  m_uFileLength = size.QuadPart;

	return TRUE;
}

BOOL CFileMemoryMapping::Destroy()
{
	if(m_pViewStartPtr != NULL)
  {
		UnmapViewOfFile(m_pViewStartPtr);    
  }
	
  if(m_hFileMapping!=NULL)
  {
	  CloseHandle(m_hFileMapping);    
  }

  if(m_hFile!=INVALID_HANDLE_VALUE)
  {
	  CloseHandle(m_hFile);    
  }

	m_hFileMapping = NULL;
	m_hFile = INVALID_HANDLE_VALUE;	
	m_pViewStartPtr = NULL;	
	
	return TRUE;
}

ULONG64 CFileMemoryMapping::GetSize()
{
	return m_uFileLength;
}

BOOL CFileMemoryMapping::Read(LPBYTE pBuffer, DWORD dwOffset, DWORD dwLength)
{	
  DWORD dwBaseOffs = dwOffset - dwOffset%m_dwAllocGranularity;
  DWORD dwDiff = dwOffset-dwBaseOffs;

  if(m_pViewStartPtr!=NULL)
    UnmapViewOfFile(m_pViewStartPtr);
	
  m_pViewStartPtr = (LPBYTE)MapViewOfFile(m_hFileMapping, FILE_MAP_READ, 0, dwBaseOffs, dwLength+dwDiff);

  memcpy(pBuffer, m_pViewStartPtr+dwDiff, dwLength);

	return TRUE;
}

//-----------------------------------------------------------------------------
// CFilePreviewCtrl implementation
//-----------------------------------------------------------------------------

CFilePreviewCtrl::CFilePreviewCtrl()
{   
  m_xChar = 0;
  m_yChar = 0;
  m_nHScrollPos = 0;
	m_nHScrollMax = 0;
	m_nMaxColsPerPage = 0;
  m_nMaxLinesPerPage = 0;
	m_nMaxDisplayWidth = 0;   
	m_uNumLines = 0;
	m_uFileLength = 0;	
  m_nVScrollPos = 0;
	m_nVScrollMax = 0;
  m_nBytesPerLine = 16; 
  m_sEmptyMsg = _T("No data to display");
  m_hFont = CreateFont(14, 7, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 
    ANTIALIASED_QUALITY, FIXED_PITCH, _T("Courier"));
}

CFilePreviewCtrl::~CFilePreviewCtrl()
{
  DeleteObject(m_hFont);
}

BOOL CFilePreviewCtrl::SetFile(CString sFileName, PreviewMode mode)
{
  UNREFERENCED_PARAMETER(mode);

  if(!m_fm.Init(sFileName))
    return FALSE;

  m_uFileLength = m_fm.GetSize();
	m_uNumLines = m_uFileLength / m_nBytesPerLine;
	
	if(m_uFileLength % m_nBytesPerLine)
		m_uNumLines++;

  CRect rcClient;
  GetClientRect(&rcClient);
  
	m_nVScrollPos = 0; 
  m_nHScrollPos = 0; 

	m_nMaxDisplayWidth = 
		8 +				//adress
		2 +				//padding
		m_nBytesPerLine * 3 +	//hex column
		1 +				//padding
		m_nBytesPerLine;	//ascii column

  	
  HDC hDC = GetDC();
	HFONT hOldFont = (HFONT)SelectObject(hDC, m_hFont);
	
  LOGFONT lf;
	ZeroMemory(&lf, sizeof(LOGFONT));
	GetObject(m_hFont, sizeof(LOGFONT), &lf);			
  m_xChar = lf.lfWidth;
  m_yChar = lf.lfHeight;
	
  SelectObject(hDC, hOldFont);

  SetupScrollbars();
	InvalidateRect(NULL, FALSE);
  UpdateWindow();

  return TRUE;
}

void CFilePreviewCtrl::SetEmptyMessage(CString sText)
{
  m_sEmptyMsg = sText;
}

BOOL CFilePreviewCtrl::SetBytesPerLine(int nBytesPerLine)
{
  if(nBytesPerLine<0)
    return FALSE;

  m_nBytesPerLine = nBytesPerLine;
  return TRUE;
}

void CFilePreviewCtrl::SetupScrollbars()
{
  CRect rcClient;
  GetClientRect(&rcClient);

  SCROLLINFO sInfo;

	//	Vertical scrollbar

  m_nMaxLinesPerPage = (int)min(m_uNumLines, rcClient.Height() / m_yChar);
	m_nVScrollMax = max(0, m_uNumLines-1);
  m_nVScrollPos = min(m_nVScrollPos, m_nVScrollMax-m_nMaxLinesPerPage+1);
	
	sInfo.cbSize = sizeof(SCROLLINFO);
	sInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	sInfo.nMin	= 0;
	sInfo.nMax	= m_nVScrollMax;
	sInfo.nPos	= m_nVScrollPos;
	sInfo.nPage	= min(m_nMaxLinesPerPage, m_nVScrollMax+1);
	SetScrollInfo (SB_VERT, &sInfo, TRUE);
	
	//	Horizontal scrollbar (not implemted by WM_HSCROLL yet...)
	
  m_nMaxColsPerPage = min(m_nMaxDisplayWidth+1, rcClient.Width() / m_xChar);	
  m_nHScrollMax = m_nMaxDisplayWidth;
  m_nHScrollPos = min(m_nHScrollPos, m_nHScrollMax-m_nMaxColsPerPage+1);
	
	sInfo.cbSize = sizeof(SCROLLINFO);
	sInfo.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
	sInfo.nMin	= 0;
	sInfo.nMax	= m_nHScrollMax;
	sInfo.nPos	= m_nHScrollPos;
	sInfo.nPage	= min(m_nMaxColsPerPage, m_nHScrollMax+1);

	SetScrollInfo (SB_HORZ, &sInfo, TRUE);

}	

//
//	Create 1 line of a hex-dump, given a buffer of BYTES
//
CString CFilePreviewCtrl::FormatHexLine(LPBYTE pData, int nBytesInLine, ULONG64 uLineOffset)
{
  CString sResult;
  CString str;
	int i;

	//print the hex address
  str.Format(_T("%08X  "), uLineOffset);
  sResult += str;

	//print hex data
	for(i = 0; i < nBytesInLine; i++)
	{
    str.Format(_T("%02X "), pData[i]);
    sResult += str;
	}

	//print some blanks if this isn't a full line
	for(; i < m_nBytesPerLine; i++)
	{
    str.Format(_T("   "));
    sResult += str;
	}

	//print a gap between the hex and ascii
	sResult += _T(" ");

	//print the ascii
	for(i = 0; i < nBytesInLine; i++)
	{
		BYTE c = pData[i];
		if(c < 32 || c > 128) c = '.';
    str.Format( _T("%c"), c);
    sResult += str;
	}

	//print some blanks if this isn't a full line
	for(; i < m_nBytesPerLine; i++)
	{
		sResult += _T(" ");
	}

  return sResult;
}

//
//	Draw 1 line to the display
//
void CFilePreviewCtrl::DrawLine(HDC hdc, DWORD nLineNo)
{
	BYTE buf[100];
	
  int nBytesPerLine = m_nBytesPerLine;

	if(m_uFileLength - nLineNo * m_nBytesPerLine < (UINT)m_nBytesPerLine)
		nBytesPerLine= m_uFileLength - nLineNo * m_nBytesPerLine;

	//get data from our file mapping
	m_fm.Read(buf, nLineNo * m_nBytesPerLine, nBytesPerLine);
	
	//convert the data into a one-line hex-dump
	CString str = FormatHexLine(buf, nBytesPerLine, nLineNo*m_nBytesPerLine );

	//draw this line to the screen
	TextOut(hdc, -(int)(m_nHScrollPos * m_xChar), 
    (nLineNo - m_nVScrollPos) * (m_yChar-1) , str, str.GetLength());
}

void CFilePreviewCtrl::DoPaintEmpty(HDC hDC)
{
  RECT rcClient;
  GetClientRect(&rcClient);

  HFONT hOldFont = (HFONT)SelectObject(hDC, m_hFont);

  FillRect(hDC, &rcClient, (HBRUSH)GetStockObject(WHITE_BRUSH));

  CRect rcText;
  DrawTextEx(hDC, m_sEmptyMsg.GetBuffer(0), -1, &rcText, DT_CALCRECT, NULL);

  rcText.MoveToX(rcClient.right/2-rcText.right/2);
  DrawTextEx(hDC, m_sEmptyMsg.GetBuffer(0), -1, &rcText, DT_LEFT, NULL);

  SelectObject(hDC, hOldFont);
}

void CFilePreviewCtrl::DoPaint(HDC hDC)
{
  HFONT hOldFont = (HFONT)SelectObject(hDC, m_hFont);
  
  RECT rcClient;
  GetClientRect(&rcClient);

  HRGN hRgn = CreateRectRgn(0, 0, rcClient.right, rcClient.bottom);
  SelectClipRgn(hDC, hRgn);
	  
  FillRect(hDC, &rcClient, (HBRUSH)GetStockObject(WHITE_BRUSH));

	int iPaintBeg = max(0, m_nVScrollPos);			//only update the lines that 
	int iPaintEnd = min(m_uNumLines, m_nVScrollPos + rcClient.bottom / m_yChar);		//need updating!!!!!!!!!!!!!
	
  
	if(rcClient.bottom % m_yChar) iPaintEnd++;
	if(iPaintEnd > m_uNumLines) iPaintEnd--;
	
	//
	//	Only paint what needs to be!
	//
  int i;
	for(i = iPaintBeg; i < iPaintEnd; i++)
	{
		DrawLine(hDC, i);
		
		//fill any extra space to the right
		if(rcClient.right > m_nMaxDisplayWidth * m_xChar)
		{
      RECT rc;
			SetRect(&rc, m_nMaxDisplayWidth * m_xChar, (i-m_nVScrollPos) * m_yChar, 
        rcClient.right, (i-m_nVScrollPos+1) * m_yChar);
			ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, _T(""), 0, 0);
		}
	}
	
	//fill any extra space below the hex dump
	if(m_nVScrollPos == m_nVScrollMax - m_nMaxLinesPerPage + 1)
	{
    RECT rc;
    SetRect(&rc, 0, m_nMaxLinesPerPage * m_yChar, rcClient.right, rcClient.bottom);
		ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, _T(""), 0, 0);
	}
	
	//if need to paint below..
	if(m_uNumLines == 0 || m_uNumLines < m_nMaxLinesPerPage)
	{
    RECT rc;
    SetRect(&rc, 0, m_uNumLines * m_yChar, rcClient.right, rcClient.bottom);
		ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rc, _T(""), 0, 0);
	}
	
	
	SelectObject(hDC, hOldFont);
}


LRESULT CFilePreviewCtrl::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
  m_fm.Destroy();
  bHandled = FALSE;  
  return 0;
}

LRESULT CFilePreviewCtrl::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
  int oldVScrollPos = m_nVScrollPos;

	SetupScrollbars();

	InvalidateRect(NULL, FALSE);
	UpdateWindow();
	

  return 0;
}

LRESULT CFilePreviewCtrl::OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  SCROLLINFO info;
	int nHScrollInc;
	int  nOldHScrollPos = m_nHScrollPos;
		
	switch (LOWORD(wParam))
	{
	case SB_LEFT:
		m_nHScrollPos = 0;
		break;

	case SB_RIGHT:
		m_nHScrollPos = m_nHScrollMax + 1;
		break;

	case SB_LINELEFT:
		if(m_nHScrollPos > 0) --m_nHScrollPos;
		break;

	case SB_LINERIGHT:
		m_nHScrollPos++;
		break;

	case SB_PAGELEFT:
		m_nHScrollPos -= 1;
		if(m_nHScrollPos > nOldHScrollPos) 
      m_nHScrollPos = 0;
		break;

	case SB_PAGERIGHT:
		m_nHScrollPos += 1;
		break;

	case SB_THUMBPOSITION:
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_TRACKPOS;
		GetScrollInfo(SB_HORZ, &info);
		m_nHScrollPos = info.nTrackPos;
		break;

	case SB_THUMBTRACK:
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_TRACKPOS;
		GetScrollInfo(SB_HORZ, &info);
		m_nHScrollPos = info.nTrackPos;
		break;

	default:
		nHScrollInc = 0;
	}

	//keep scroll position in range
	if(m_nHScrollPos > m_nHScrollMax)
		m_nHScrollPos = m_nHScrollMax;

	nHScrollInc = m_nHScrollPos - nOldHScrollPos;
		
	if (nHScrollInc)
	{	
		
		//finally setup the actual scrollbar!
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_POS;
		info.nPos = m_nHScrollPos;
		SetScrollInfo(SB_HORZ, &info, TRUE);
				
		InvalidateRect(NULL);
	}
		
  return 0;
}

LRESULT CFilePreviewCtrl::OnVScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  // React to the various vertical scroll related actions.
	// CAUTION:
	// All sizes are in unsigned values, so be carefull
	// when testing for < 0 etc

	SCROLLINFO info;
	int nVScrollInc;
	int  nOldVScrollPos = m_nVScrollPos;
		
	//HideCaret();

	switch (LOWORD(wParam))
	{
	case SB_TOP:
		m_nVScrollPos = 0;
		break;

	case SB_BOTTOM:
		m_nVScrollPos = m_nVScrollMax - m_nMaxLinesPerPage + 1;
		break;

	case SB_LINEUP:
		if(m_nVScrollPos > 0) --m_nVScrollPos;
		break;

	case SB_LINEDOWN:
		m_nVScrollPos++;
		break;

	case SB_PAGEUP:
		m_nVScrollPos -= max(1, m_nMaxLinesPerPage);
		if(m_nVScrollPos > nOldVScrollPos) m_nVScrollPos = 0;
		break;

	case SB_PAGEDOWN:
		m_nVScrollPos += max(1, m_nMaxLinesPerPage);
		break;

	case SB_THUMBPOSITION:
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_TRACKPOS;
		GetScrollInfo(SB_VERT, &info);
		m_nVScrollPos = info.nTrackPos;
		break;

	case SB_THUMBTRACK:
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_TRACKPOS;
		GetScrollInfo(SB_VERT, &info);
		m_nVScrollPos = info.nTrackPos;
		break;

	default:
		nVScrollInc = 0;
	}

	//keep scroll position in range
	if(m_nVScrollPos > m_nVScrollMax - m_nMaxLinesPerPage+1)
		m_nVScrollPos = m_nVScrollMax - m_nMaxLinesPerPage+1;

	nVScrollInc = m_nVScrollPos - nOldVScrollPos;
		
	if (nVScrollInc)
	{	
		
		//finally setup the actual scrollbar!
		info.cbSize = sizeof(SCROLLINFO);
		info.fMask = SIF_POS;
		info.nPos = m_nVScrollPos;
		SetScrollInfo(SB_VERT, &info, TRUE);
				
		InvalidateRect(NULL);
	}
	
	return 0;
}

LRESULT CFilePreviewCtrl::OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  // Do nothing
	return 0;   
}

LRESULT CFilePreviewCtrl::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{ 
  PAINTSTRUCT ps;
	HDC hDC = BeginPaint(&ps);

  {
    CMemoryDC memDC(hDC, ps.rcPaint);

    if(m_fm.GetSize()==0)
      DoPaintEmpty(memDC);
    else
      DoPaint(memDC);
  }
  	
	EndPaint(&ps);
	
  return 0;
}

