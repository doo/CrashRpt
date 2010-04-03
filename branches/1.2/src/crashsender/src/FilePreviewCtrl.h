#pragma once
#include "atlmisc.h"

// Preview mode
enum PreviewMode
{
  PREVIEW_HEX,  // Hex
  PREVIEW_TEXT, // Text
  PREVIEW_PNG   // PNG
};

// Used to map file contents into memory
class CFileMemoryMapping
{
public:

	CFileMemoryMapping();  
  ~CFileMemoryMapping();  

	BOOL Init(LPCTSTR szFileName);
	BOOL Destroy();

  ULONG64 GetSize();
	BOOL Read(LPBYTE pBuffer, DWORD dwOffset, DWORD dwLength);	

private:
	
  HANDLE m_hFile;		          // Handle to current file
	HANDLE m_hFileMapping;		  // Memory mapped object
  DWORD m_dwAllocGranularity; // System allocation granularity  	
	ULONG64 m_uFileLength;		  // Size of the file.		
	LPBYTE m_pViewStartPtr;	    // Base of the view of the file.  
};

class CFilePreviewCtrl : public CWindowImpl<CFilePreviewCtrl, CStatic>
{
public:
  
  CFilePreviewCtrl();
  ~CFilePreviewCtrl();

  DECLARE_WND_SUPERCLASS(NULL, CWindow::GetWndClassName())

  BEGIN_MSG_MAP(CFilePreviewCtrl)  
    if (uMsg == WM_NCHITTEST || 
        uMsg == WM_NCLBUTTONDOWN || 
        uMsg == WM_NCMBUTTONDOWN ||
        uMsg == WM_NCXBUTTONDOWN ||
        uMsg == WM_NCLBUTTONDBLCLK)
    {
      // This is to enable scroll bar messages
      bHandled = TRUE;
      lResult = ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
      return TRUE;
    }

    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
    MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
  END_MSG_MAP()

  BOOL SetFile(CString sFileName, PreviewMode mode);
  void SetEmptyMessage(CString sText);
  BOOL SetBytesPerLine(int nBytesPerLine);

  LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnVScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  void SetupScrollbars();
  void DrawLine(HDC hdc, DWORD nLineNo);
  CString FormatHexLine(LPBYTE pData, int nBytesInLine, ULONG64 uLineOffset);
  void DoPaintEmpty(HDC hDC);
  void DoPaint(HDC hDC);

  CFileMemoryMapping m_fm;  // File mapping object.  
  HFONT m_hFont;            // Font in use.  
  int m_xChar;              // Size of character in x direction.
  int m_yChar;              // Size of character in y direction.
  int m_nMaxColsPerPage;    // Maximum columns per page.
  int m_nMaxLinesPerPage;   // Maximum count of lines per one page.
	int m_nMaxDisplayWidth;   
	ULONG64 m_uNumLines;      // Number of lines in the file.
	ULONG64 m_uFileLength;    // Length of file in bytes.
  int m_nBytesPerLine;      // Count of displayed bytes per line.
  CString m_sEmptyMsg;      // Text to display when file is empty
  int m_nHScrollPos;        // Horizontal scroll position.
	int m_nHScrollMax;        // Max horizontal scroll position.
	int m_nVScrollPos;        // Vertical scrolling position.
	int m_nVScrollMax;        // Maximum vertical scrolling position.
};



