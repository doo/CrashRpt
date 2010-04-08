#include "stdafx.h"
#include "FilePreviewCtrl.h"
#include <set>

//-----------------------------------------------------------------------------
// CFileMemoryMapping implementation
//-----------------------------------------------------------------------------

CFileMemoryMapping::CFileMemoryMapping()  
{
  m_hFile = INVALID_HANDLE_VALUE;
  m_uFileLength = 0;
  m_hFileMapping = NULL;  
  
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
  std::map<DWORD, LPBYTE>::iterator it;
  for(it=m_aViewStartPtrs.begin(); it!=m_aViewStartPtrs.end(); it++)
  {
    if(it->second != NULL)
      UnmapViewOfFile(it->second);    
  }
  m_aViewStartPtrs.clear();
  
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
	m_uFileLength = 0;  

	return TRUE;
}

ULONG64 CFileMemoryMapping::GetSize()
{
	return m_uFileLength;
}

LPBYTE CFileMemoryMapping::CreateView(DWORD dwOffset, DWORD dwLength)
{
  DWORD dwThreadId = GetCurrentThreadId();
  DWORD dwBaseOffs = dwOffset-dwOffset%m_dwAllocGranularity;
  DWORD dwDiff = dwOffset-dwBaseOffs;
  LPBYTE pPtr = NULL;

  CAutoLock lock(&m_csLock);

  std::map<DWORD, LPBYTE>::iterator it = m_aViewStartPtrs.find(dwThreadId);
  if(it!=m_aViewStartPtrs.end())
  {
    UnmapViewOfFile(it->second);
  }
  
  pPtr = (LPBYTE)MapViewOfFile(m_hFileMapping, FILE_MAP_READ, 0, dwBaseOffs, dwLength+dwDiff);
  if(it!=m_aViewStartPtrs.end())
  {
    it->second = pPtr;
  }
  else
  {
    m_aViewStartPtrs[dwThreadId] = pPtr;
  }
  
  return (pPtr+dwDiff);
}

//-----------------------------------------------------------------------------
// CFilePreviewCtrl implementation
//-----------------------------------------------------------------------------

CFilePreviewCtrl::CFilePreviewCtrl()
{   
  m_xChar = 10;
  m_yChar = 10;
  m_nHScrollPos = 0;
	m_nHScrollMax = 0;
	m_nMaxColsPerPage = 0;
  m_nMaxLinesPerPage = 0;
	m_nMaxDisplayWidth = 0;   
	m_uNumLines = 0;
  m_nVScrollPos = 0;
	m_nVScrollMax = 0;
  m_nBytesPerLine = 16; 
  m_cchTabLength = 4;
  m_sEmptyMsg = _T("No data to display");
  m_hFont = CreateFont(14, 7, 0, 0, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 
    ANTIALIASED_QUALITY, FIXED_PITCH, _T("Courier"));

  m_hWorkerThread = NULL;
  m_bCancelled = FALSE;
  m_PreviewMode = PREVIEW_HEX;
}

CFilePreviewCtrl::~CFilePreviewCtrl()
{
  DeleteObject(m_hFont);  
}

LPCTSTR CFilePreviewCtrl::GetFile()
{
  if(m_sFileName.IsEmpty())
    return NULL;
  return m_sFileName;
}

BOOL CFilePreviewCtrl::SetFile(LPCTSTR szFileName, PreviewMode mode)
{
  // If we are currently processing some file in background,
  // stop the worker thread
  if(m_hWorkerThread!=NULL)
  {
    m_bCancelled = TRUE;
    WaitForSingleObject(m_hWorkerThread, INFINITE);
    m_hWorkerThread = NULL;
  }

  CAutoLock lock(&m_csLock);

  m_sFileName = szFileName;
  
  if(mode==PREVIEW_AUTO)
    m_PreviewMode = DetectPreviewMode(m_sFileName);
  else
    m_PreviewMode = mode;
  
  if(szFileName==NULL)
  {
    m_fm.Destroy();
  }
  else
  {
    if(!m_fm.Init(m_sFileName))
    {
      m_sFileName.Empty();
      return FALSE;
    }
  }

  CRect rcClient;
  GetClientRect(&rcClient);

  HDC hDC = GetDC();
	HFONT hOldFont = (HFONT)SelectObject(hDC, m_hFont);
	
  LOGFONT lf;
	ZeroMemory(&lf, sizeof(LOGFONT));
	GetObject(m_hFont, sizeof(LOGFONT), &lf);			
  m_xChar = lf.lfWidth;
  m_yChar = lf.lfHeight;
	
  SelectObject(hDC, hOldFont);

	m_nVScrollPos = 0; 
  m_nVScrollMax = 0;
  m_nHScrollPos = 0;
  m_nHScrollMax = 0;
  m_aTextLines.clear();
  m_uNumLines = 0;
  m_nMaxDisplayWidth = 0;

  if(m_PreviewMode==PREVIEW_HEX)
  {
    if(m_fm.GetSize()!=0)
    {
	    m_nMaxDisplayWidth = 
		    8 +				//adress
		    2 +				//padding
		    m_nBytesPerLine * 3 +	//hex column
		    1 +				//padding
		    m_nBytesPerLine;	//ascii column
    }

    m_uNumLines = m_fm.GetSize() / m_nBytesPerLine;
	
    if(m_fm.GetSize() % m_nBytesPerLine)
		  m_uNumLines++;
  }
  else if(m_PreviewMode==PREVIEW_TEXT)
  {       
    m_bCancelled = FALSE;
    m_hWorkerThread = CreateThread(NULL, 0, TextParsingThread, this, 0, NULL);
    SetTimer(0, 250, NULL);
  }

  SetupScrollbars();
	InvalidateRect(NULL, FALSE);
  UpdateWindow();

  return TRUE;
}

PreviewMode CFilePreviewCtrl::GetPreviewMode()
{
  return m_PreviewMode;
}

void CFilePreviewCtrl::SetPreviewMode(PreviewMode mode)
{
  SetFile(m_sFileName, mode);
}

PreviewMode CFilePreviewCtrl::DetectPreviewMode(LPCTSTR szFileName)
{
  PreviewMode mode = PREVIEW_HEX;

  CString sFileName = szFileName;
  int backslash_pos = sFileName.ReverseFind('\\');
  if(backslash_pos>=0)
    sFileName = sFileName.Mid(backslash_pos+1);
  CString sExtension;
  int dot_pos = sFileName.ReverseFind('.');
  if(dot_pos>0)
    sExtension = sFileName.Mid(dot_pos+1);
  sExtension.MakeUpper();

  std::set<CString> aTextFileExtensions;
  aTextFileExtensions.insert(_T("TXT"));
  aTextFileExtensions.insert(_T("INI"));
  aTextFileExtensions.insert(_T("LOG"));
  aTextFileExtensions.insert(_T("XML"));
  aTextFileExtensions.insert(_T("HTM"));
  aTextFileExtensions.insert(_T("HTML"));
  aTextFileExtensions.insert(_T("JS"));
  aTextFileExtensions.insert(_T("C"));
  aTextFileExtensions.insert(_T("H"));
  aTextFileExtensions.insert(_T("CPP"));
  aTextFileExtensions.insert(_T("HPP"));

  std::set<CString>::iterator it = aTextFileExtensions.find(sExtension);
  if(it!=aTextFileExtensions.end())
  {
    mode = PREVIEW_TEXT;
  }

  return mode;
}

DWORD WINAPI CFilePreviewCtrl::TextParsingThread(LPVOID lpParam)
{
  CFilePreviewCtrl* pCtrl = (CFilePreviewCtrl*)lpParam;
  pCtrl->ParseText();
  
  return 0;
}

void CFilePreviewCtrl::ParseText()
{
  DWORD dwFileSize = (DWORD)m_fm.GetSize();
  DWORD dwOffset = 0;
  DWORD dwPrevOffset = 0;
  int nTabs = 0;  

  if(dwFileSize!=0)
  {
      CAutoLock lock(&m_csLock);        
      m_aTextLines.push_back(0);
  }

  for(;;)
  {
    {
      CAutoLock lock(&m_csLock);        
      if(m_bCancelled)
        break;
    }

    DWORD dwLength = 4096;
    if(dwOffset+dwLength>=dwFileSize)
      dwLength = dwFileSize-dwOffset;

    if(dwLength==0)
      break;

    LPBYTE ptr = m_fm.CreateView(dwOffset, dwLength);

    UINT i;
    for(i=0; i<dwLength; i++)
    {
      {
        CAutoLock lock(&m_csLock);        
        if(m_bCancelled)
          break;
      }

      char c = ((char*)ptr)[i];

      if(c=='\t')
      {
        nTabs++;
      }
      else if(c=='\n')
      {
        CAutoLock lock(&m_csLock);        
        m_aTextLines.push_back(dwOffset+i+1);
        int cchLineLength = dwOffset+i+1-dwPrevOffset;
        if(nTabs!=0)
          cchLineLength += nTabs*(m_cchTabLength-1);

        m_nMaxDisplayWidth = max(m_nMaxDisplayWidth, cchLineLength);
        m_uNumLines++;
        dwPrevOffset = dwOffset+i+1;        
        nTabs = 0;
      }
    }

    dwOffset += dwLength;        
  }

  PostMessage(WM_FPC_COMPLETE);
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
  CAutoLock lock(&m_csLock);
  CRect rcClient;
  GetClientRect(&rcClient);

  SCROLLINFO sInfo;

	//	Vertical scrollbar

  m_nMaxLinesPerPage = (int)min(m_uNumLines, rcClient.Height() / m_yChar);
	m_nVScrollMax = (int)max(0, m_uNumLines-1);
  m_nVScrollPos = (int)min(m_nVScrollPos, m_nVScrollMax-m_nMaxLinesPerPage+1);
	
	sInfo.cbSize = sizeof(SCROLLINFO);
	sInfo.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	sInfo.nMin	= 0;
	sInfo.nMax	= m_nVScrollMax;
	sInfo.nPos	= m_nVScrollPos;
	sInfo.nPage	= min(m_nMaxLinesPerPage, m_nVScrollMax+1);
	SetScrollInfo (SB_VERT, &sInfo, TRUE);
	
	//	Horizontal scrollbar 
	
  m_nMaxColsPerPage = min(m_nMaxDisplayWidth+1, rcClient.Width() / m_xChar);	
  m_nHScrollMax = max(0, m_nMaxDisplayWidth-1);
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
  int nBytesPerLine = m_nBytesPerLine;

  if(m_fm.GetSize() - nLineNo * m_nBytesPerLine < (UINT)m_nBytesPerLine)
    nBytesPerLine= (DWORD)m_fm.GetSize() - nLineNo * m_nBytesPerLine;

	//get data from our file mapping
	LPBYTE ptr = m_fm.CreateView(nLineNo * m_nBytesPerLine, nBytesPerLine);
	
	//convert the data into a one-line hex-dump
	CString str = FormatHexLine(ptr, nBytesPerLine, nLineNo*m_nBytesPerLine );

	//draw this line to the screen
	TextOut(hdc, -(int)(m_nHScrollPos * m_xChar), 
    (nLineNo - m_nVScrollPos) * (m_yChar-1) , str, str.GetLength());
}

void CFilePreviewCtrl::DrawTextLine(HDC hdc, DWORD nLineNo)
{
  CRect rcClient;
  GetClientRect(&rcClient);

  DWORD dwOffset = 0;
  DWORD dwLength = 0;
  {
    CAutoLock lock(&m_csLock);
    dwOffset = m_aTextLines[nLineNo];
    if(nLineNo==m_uNumLines-1)
      dwLength = (DWORD)m_fm.GetSize() - dwOffset;
    else
      dwLength = m_aTextLines[nLineNo+1]-dwOffset-1;
  }

  if(dwLength==0)
    return;
  
  //get data from our file mapping
	LPBYTE ptr = m_fm.CreateView(dwOffset, dwLength);

  //draw this line to the screen
  CRect rcText;
  rcText.left = -(int)(m_nHScrollPos * m_xChar);
  rcText.top = (nLineNo - m_nVScrollPos) * m_yChar;
  rcText.right = rcClient.right;
  rcText.bottom = rcText.top + m_yChar;
  DRAWTEXTPARAMS params;
  memset(&params, 0, sizeof(DRAWTEXTPARAMS));
  params.cbSize = sizeof(DRAWTEXTPARAMS);
  params.iTabLength = m_xChar*m_cchTabLength;

  DWORD dwFlags = DT_LEFT|DT_TOP|DT_SINGLELINE|DT_NOPREFIX|DT_EXPANDTABS;
	  
  DrawTextExA(hdc, (char*)ptr, dwLength-1,  &rcText, 
    dwFlags, &params);
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
	int iPaintEnd = (int)min(m_uNumLines, m_nVScrollPos + rcClient.bottom / m_yChar);		//need updating!!!!!!!!!!!!!
	
  
	if(rcClient.bottom % m_yChar) iPaintEnd++;
	if(iPaintEnd > m_uNumLines) iPaintEnd--;
	
	//
	//	Only paint what needs to be!
	//
  int i;
	for(i = iPaintBeg; i < iPaintEnd; i++)
	{
    if(m_PreviewMode==PREVIEW_HEX)
      DrawLine(hDC, i);
    else
      DrawTextLine(hDC, i);				
	}
		
	SelectObject(hDC, hOldFont);
}

LRESULT CFilePreviewCtrl::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
  m_fm.Destroy();
  bHandled = FALSE;  
  return 0;
}

LRESULT CFilePreviewCtrl::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SetupScrollbars();

	InvalidateRect(NULL, FALSE);
	UpdateWindow();

  return 0;
}

LRESULT CFilePreviewCtrl::OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  SCROLLINFO info;
	int nHScrollInc = 0;
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
		m_nHScrollPos -= m_nMaxColsPerPage;
		if(m_nHScrollPos > nOldHScrollPos) 
      m_nHScrollPos = 0;
		break;

	case SB_PAGERIGHT:
		m_nHScrollPos += m_nMaxColsPerPage;
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
	if(m_nHScrollPos  > m_nHScrollMax - m_nMaxColsPerPage + 1)
		m_nHScrollPos = m_nHScrollMax - m_nMaxColsPerPage + 1;
  
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

  if(m_nVScrollPos<0)
    m_nVScrollPos = 0;

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

LRESULT CFilePreviewCtrl::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  SetupScrollbars();
  InvalidateRect(NULL);
  return 0;
}

LRESULT CFilePreviewCtrl::OnComplete(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  KillTimer(0);
  SetupScrollbars();
  InvalidateRect(NULL);
  return 0;
}