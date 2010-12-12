/************************************************************************************* 
  This file is a part of CrashRpt library.

  Copyright (c) 2003, Michael Carruth
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

#ifndef __SCREENCAP_H__
#define __SCREENCAP_H__

#include "stdafx.h"

extern "C" {
#include "png.h"
}

struct WindowInfo
{
  CString m_sTitle; // Window title
  CRect m_rcWnd;    // Window rect
};

class CScreenCapture
{
public:

  CScreenCapture();
  ~CScreenCapture();

  // Returns virtual screen rectangle
  void GetScreenRect(LPRECT rcScreen);

  // Returns an array of visible windows for the specified process or 
  // the main window of the process.
  BOOL FindWindows(HANDLE hProcess, BOOL bAllProcessWindows, std::vector<WindowInfo>* paWindows);

  // Captures the specified screen area and returns the list of image files
  BOOL CaptureScreenRect(std::vector<CRect> arcCapture, CString sSaveDirName, 
    int nIdStartFrom, std::vector<CString>& out_file_list);

  /* PNG management functions */

  // Initializes PNG file header
  BOOL PngInit(int nWidth, int nHeight, CString sFileName);
  // Writes a scan line to the PNG file
  BOOL PngWriteRow(LPBYTE pRow);
  // Closes PNG file
  BOOL PngFinalize();

  /* Member variables. */

  CPoint m_ptCursorPos;                 // Current mouse cursor pos
  std::vector<CRect> m_arcCapture;      // Array of capture rectangles
  CURSORINFO m_CursorInfo;              // Cursor info
  int m_nIdStartFrom;                   // An ID for the current screenshot image 
  CString m_sSaveDirName;               // Directory name to save screenshots to
  FILE* m_fp;                           // Handle to the file
  png_structp m_png_ptr;                // libpng stuff
  png_infop m_info_ptr;                 // libpng stuff
  std::vector<CString> m_out_file_list; // The list of output image files
};

#endif //__SCREENCAP_H__


