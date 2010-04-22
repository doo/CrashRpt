#pragma once
#include "atlmisc.h"
#include <vector>
#include <map>
#include "CritSec.h"

// Preview mode
enum PreviewMode
{
  PREVIEW_AUTO = -1,  // Auto
  PREVIEW_HEX  = 0,   // Hex
  PREVIEW_TEXT = 1,   // Text
  PREVIEW_BITMAP = 2, // Bitmap
  PREVIEW_PNG  = 3    // PNG
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
	LPBYTE CreateView(DWORD dwOffset, DWORD dwLength);

private:

  HANDLE m_hFile;		          // Handle to current file
	HANDLE m_hFileMapping;		  // Memory mapped object
  DWORD m_dwAllocGranularity; // System allocation granularity  	  
	ULONG64 m_uFileLength;		  // Size of the file.		
  CCritSec m_csLock;
  std::map<DWORD, LPBYTE> m_aViewStartPtrs; // Base of the view of the file.    
};

struct LineInfo
{
  DWORD m_dwOffsetInFile;
  DWORD m_cchLineLength;  
};

class CDiBitmap
{
public:

  CDiBitmap();
  ~CDiBitmap();

  static BOOL IsBitmap(FILE* f);
  BOOL Create(int nWidth, int nHeight, int nBitsPerPixel);
  BOOL Load(CString sFileName);
  BOOL Resize(CDiBitmap* pDstBitmap);
  
private:

  void Init();
  void  SetBitmapInfo(BITMAPINFO* bmi, int nWidth, int nHeight, int nBitsPerPixel);

  BITMAPINFO m_bmi;       // Bitmap info.
  HBITMAP m_hBitmap;      // Handle to the bitmap.
  HBITMAP m_hOldBitmap;	  // Old bitmap 1x1.
	LPBYTE  m_pbDiBits;     // Buffer for DIB bits.
  DWORD   m_dwDibSize;    // Size of the DIB data.
};

#define WM_FPC_COMPLETE  (WM_APP+100)

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
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_FPC_COMPLETE, OnComplete)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
  END_MSG_MAP()

  LPCTSTR GetFile();
  BOOL SetFile(LPCTSTR szFileName, PreviewMode mode=PREVIEW_AUTO);
  PreviewMode GetPreviewMode();
  void SetPreviewMode(PreviewMode mode);
  void SetEmptyMessage(CString sText);
  BOOL SetBytesPerLine(int nBytesPerLine);

  PreviewMode DetectPreviewMode(LPCTSTR szFileName);

  LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnHScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnVScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnComplete(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnRButtonUp(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  void SetupScrollbars();
  CString FormatHexLine(LPBYTE pData, int nBytesInLine, ULONG64 uLineOffset);
  void DrawLine(HDC hdc, DWORD nLineNo);
  void DrawTextLine(HDC hdc, DWORD nLineNo);  
  void DoPaintEmpty(HDC hDC);
  void DoPaint(HDC hDC);

  static DWORD WINAPI TextParsingThread(LPVOID lpParam);
  void ParseText();
  
  BOOL IsPNG(FILE* f);
  
  CString m_sFileName;
  PreviewMode m_PreviewMode;
  CCritSec m_csLock;
  CFileMemoryMapping m_fm;  // File mapping object.  
  HFONT m_hFont;            // Font in use.  
  int m_xChar;              // Size of character in x direction.
  int m_yChar;              // Size of character in y direction.
  int m_nMaxColsPerPage;    // Maximum columns per page.
  int m_nMaxLinesPerPage;   // Maximum count of lines per one page.
	int m_nMaxDisplayWidth;   
	ULONG64 m_uNumLines;      // Number of lines in the file.	
  int m_nBytesPerLine;      // Count of displayed bytes per line.
  int m_cchTabLength;
  CString m_sEmptyMsg;      // Text to display when file is empty
  int m_nHScrollPos;        // Horizontal scroll position.
	int m_nHScrollMax;        // Max horizontal scroll position.
	int m_nVScrollPos;        // Vertical scrolling position.
	int m_nVScrollMax;        // Maximum vertical scrolling position.  
  std::vector<DWORD> m_aTextLines;
  HANDLE m_hWorkerThread;
  BOOL m_bCancelled;
  CBitmap m_bmp;
};



