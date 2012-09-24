/************************************************************************************* 
This file is a part of CrashRpt library.
Copyright (c) 2010 The CrashRpt project authors. All Rights Reserved.

Use of this source code is governed by a BSD-style license
that can be found in the License.txt file in the root of the source
tree. All contributing project authors may
be found in the Authors.txt file in the root of the source tree.
***************************************************************************************/

// File: VideoRec.h
// Description: Video recording functionality.
// Authors: zexspectrum
// Date: Sep 2012

#pragma once
#include "stdafx.h"
#include "ScreenCap.h"
#include "vpx/vpx_encoder.h"

// class CVideoRecorder
// Captures desktop and writes the video frames as raw uncompressed BMP files
// Later the recorded frames are encoded to a VP8-encoded webm video file.
//
class CVideoRecorder
{
public:

	// Constructor
	CVideoRecorder();

	// Destructor
	~CVideoRecorder();

	// Inits the video recorder object.
	BOOL Init(LPCTSTR szSaveToDir, 
			SCREENSHOT_TYPE type,
			DWORD dwProcessId,
			int nVideoDuration,
			int nVideoFrameInterval,
			int nVideoQuality,
			SIZE* pDesiredFrameSize
			);

	// Records a single video frame
	BOOL RecordVideoFrame();

	// Encodes the video with VP8 codec and writes .webm file.
	BOOL EncodeVideo();

	// Returns the output file name
	CString GetOutFile();

private:

	// Sets video frame parameters.
	void SetVideoFrameInfo(int nFrameId, ScreenshotInfo& ssi);

	// Loads a BMP file and returns its data.
	BOOL LoadImageFromBMPFile(LPCTSTR szFileName, vpx_image_t *pImage);

	/* Internal variables */
	CString m_sSaveToDir; // Directory where to save recorded video frames.
	CString m_sOutFile;   // Output webm file.
	SCREENSHOT_TYPE m_ScreenshotType; // What part of desktop is captured.
	CScreenCapture m_sc;  // Screen capture object
	std::vector<ScreenshotInfo> m_aVideoFrames; // Array of recorded video frames.
	SIZE m_DesiredFrameSize; // Desired frame size.
	SIZE m_ActualFrameSize;  // Actual frame size.
	int m_nVideoQuality;  // Video quality.
	int m_nVideoDuration; // Video duration (in msec)
	int m_nVideoFrameInterval; // Interval between two subsequent frames (in msec).
	DWORD m_dwProcessId;  // ID of the process being captured.
	int m_nFrameCount;    // Total max count of frames.
	int m_nFileId;        // Index of current BMP file.
	int m_nFrameId;       // Index of current video frame.
	
};
