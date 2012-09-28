/************************************************************************************* 
This file is a part of CrashRpt library.
Copyright (c) 2010 The CrashRpt project authors. All Rights Reserved.

Use of this source code is governed by a BSD-style license
that can be found in the License.txt file in the root of the source
tree. All contributing project authors may
be found in the Authors.txt file in the root of the source tree.
***************************************************************************************/

// File: VideoRec.cpp
// Description: Video recording functionality.
// Authors: zexspectrum
// Date: Sep 2012

#include "stdafx.h"
#include "VideoRec.h"
#include "Utility.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"
#include "libmkv/EbmlWriter.h"
#include "libmkv/EbmlIDs.h"

#define interface (vpx_codec_vp8_cx())

/* Need special handling of these functions on Windows */
#if defined(_MSC_VER)
/* MSVS doesn't define off_t, and uses _f{seek,tell}i64 */
//typedef __int64 off_t;
#define fseeko _fseeki64
#define ftello _ftelli64
#elif defined(_WIN32)
/* MinGW defines off_t as long
   and uses f{seek,tell}o64/off64_t for large files */
#define fseeko fseeko64
#define ftello ftello64
#define off_t off64_t
#endif

#if defined(_MSC_VER)
#define LITERALU64(n) n
#else
#define LITERALU64(n) n##LLU
#endif

typedef off_t EbmlLoc;

struct EbmlGlobal
{
    int debug;

    FILE    *stream;
    int64_t last_pts_ms;
    vpx_rational_t  framerate;

    /* These pointers are to the start of an element */
    off_t    position_reference;
    off_t    seek_info_pos;
    off_t    segment_info_pos;
    off_t    track_pos;
    off_t    cue_pos;
    off_t    cluster_pos;

    /* This pointer is to a specific element to be serialized */
    off_t    track_id_pos;

    /* These pointers are to the size field of the element */
    EbmlLoc  startSegment;
    EbmlLoc  startCluster;

    uint32_t cluster_timecode;
    int      cluster_open;

    struct cue_entry *cue_list;
    unsigned int      cues;

};

/* Stereo 3D packed frame format */
typedef enum stereo_format
{
    STEREO_FORMAT_MONO       = 0,
    STEREO_FORMAT_LEFT_RIGHT = 1,
    STEREO_FORMAT_BOTTOM_TOP = 2,
    STEREO_FORMAT_TOP_BOTTOM = 3,
    STEREO_FORMAT_RIGHT_LEFT = 11
} stereo_format_t;

struct cue_entry
{
    unsigned int time;
    uint64_t     loc;
};

static void
Ebml_StartSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc,
                          unsigned long class_id)
{
    //todo this is always taking 8 bytes, this may need later optimization
    //this is a key that says length unknown
    uint64_t unknownLen =  LITERALU64(0x01FFFFFFFFFFFFFF);

    Ebml_WriteID(glob, class_id);
    *ebmlLoc = ftello(glob->stream);
    Ebml_Serialize(glob, &unknownLen, sizeof(unknownLen), 8);
}

static void
Ebml_EndSubElement(EbmlGlobal *glob, EbmlLoc *ebmlLoc)
{
    off_t pos;
    uint64_t size;

    /* Save the current stream pointer */
    pos = ftello(glob->stream);

    /* Calculate the size of this element */
    size = pos - *ebmlLoc - 8;
    size |=  LITERALU64(0x0100000000000000);

    /* Seek back to the beginning of the element and write the new size */
    fseeko(glob->stream, *ebmlLoc, SEEK_SET);
    Ebml_Serialize(glob, &size, sizeof(size), 8);

    /* Reset the stream pointer */
    fseeko(glob->stream, pos, SEEK_SET);
}

/* Need a fixed size serializer for the track ID. libmkv provides a 64 bit
 * one, but not a 32 bit one.
 */
static void Ebml_SerializeUnsigned32(EbmlGlobal *glob, unsigned long class_id, uint64_t ui)
{
    unsigned char sizeSerialized = 4 | 0x80;
    Ebml_WriteID(glob, class_id);
    Ebml_Serialize(glob, &sizeSerialized, sizeof(sizeSerialized), 1);
    Ebml_Serialize(glob, &ui, sizeof(ui), 4);
}
#define WRITE_BUFFER(s) \
for(i = len-1; i>=0; i--)\
{ \
    x = *(const s *)buffer_in >> (i * CHAR_BIT); \
    Ebml_Write(glob, &x, 1); \
}
void Ebml_Serialize(EbmlGlobal *glob, const void *buffer_in, int buffer_size, unsigned long len)
{
    char x;
    int i;

    /* buffer_size:
     * 1 - int8_t;
     * 2 - int16_t;
     * 3 - int32_t;
     * 4 - int64_t;
     */
    switch (buffer_size)
    {
        case 1:
            WRITE_BUFFER(int8_t)
            break;
        case 2:
            WRITE_BUFFER(int16_t)
            break;
        case 4:
            WRITE_BUFFER(int32_t)
            break;
        case 8:
            WRITE_BUFFER(int64_t)
            break;
        default:
            break;
    }
}
#undef WRITE_BUFFER

void Ebml_Write(EbmlGlobal *glob, const void *buffer_in, unsigned long len)
{
    if(fwrite(buffer_in, 1, len, glob->stream));
}

static void
write_webm_seek_element(EbmlGlobal *ebml, unsigned long id, off_t pos)
{
    uint64_t offset = pos - ebml->position_reference;
    EbmlLoc start;
    Ebml_StartSubElement(ebml, &start, Seek);
    Ebml_SerializeBinary(ebml, SeekID, id);
    Ebml_SerializeUnsigned64(ebml, SeekPosition, offset);
    Ebml_EndSubElement(ebml, &start);
}

static void
write_webm_seek_info(EbmlGlobal *ebml)
{

    off_t pos;

    /* Save the current stream pointer */
    pos = ftello(ebml->stream);

    if(ebml->seek_info_pos)
        fseeko(ebml->stream, ebml->seek_info_pos, SEEK_SET);
    else
        ebml->seek_info_pos = pos;

    {
        EbmlLoc start;

        Ebml_StartSubElement(ebml, &start, SeekHead);
        write_webm_seek_element(ebml, Tracks, ebml->track_pos);
        write_webm_seek_element(ebml, Cues,   ebml->cue_pos);
        write_webm_seek_element(ebml, Info,   ebml->segment_info_pos);
        Ebml_EndSubElement(ebml, &start);
    }
    {
        //segment info
        EbmlLoc startInfo;
        uint64_t frame_time;
        char version_string[64];

        /* Assemble version string */
        if(ebml->debug)
            strcpy(version_string, "vpxenc");
        else
        {
            strcpy(version_string, "vpxenc ");
            strncat(version_string,
                    vpx_codec_version_str(),
                    sizeof(version_string) - 1 - strlen(version_string));
        }

        frame_time = (uint64_t)1000 * ebml->framerate.den
                     / ebml->framerate.num;
        ebml->segment_info_pos = ftello(ebml->stream);
        Ebml_StartSubElement(ebml, &startInfo, Info);
        Ebml_SerializeUnsigned(ebml, TimecodeScale, 1000000);
        Ebml_SerializeFloat(ebml, Segment_Duration,
                            ebml->last_pts_ms + frame_time);
        Ebml_SerializeString(ebml, 0x4D80, version_string);
        Ebml_SerializeString(ebml, 0x5741, version_string);
        Ebml_EndSubElement(ebml, &startInfo);
    }
}

static void
write_webm_file_header(EbmlGlobal                *glob,
                       const vpx_codec_enc_cfg_t *cfg,
                       const struct vpx_rational *fps,
                       stereo_format_t            stereo_fmt)
{
    {
        EbmlLoc start;
        Ebml_StartSubElement(glob, &start, EBML);
        Ebml_SerializeUnsigned(glob, EBMLVersion, 1);
        Ebml_SerializeUnsigned(glob, EBMLReadVersion, 1); //EBML Read Version
        Ebml_SerializeUnsigned(glob, EBMLMaxIDLength, 4); //EBML Max ID Length
        Ebml_SerializeUnsigned(glob, EBMLMaxSizeLength, 8); //EBML Max Size Length
        Ebml_SerializeString(glob, DocType, "webm"); //Doc Type
        Ebml_SerializeUnsigned(glob, DocTypeVersion, 2); //Doc Type Version
        Ebml_SerializeUnsigned(glob, DocTypeReadVersion, 2); //Doc Type Read Version
        Ebml_EndSubElement(glob, &start);
    }
    {
        Ebml_StartSubElement(glob, &glob->startSegment, Segment); //segment
        glob->position_reference = ftello(glob->stream);
        glob->framerate = *fps;
        write_webm_seek_info(glob);

        {
            EbmlLoc trackStart;
            glob->track_pos = ftello(glob->stream);
            Ebml_StartSubElement(glob, &trackStart, Tracks);
            {
                unsigned int trackNumber = 1;
                uint64_t     trackID = 0;

                EbmlLoc start;
                Ebml_StartSubElement(glob, &start, TrackEntry);
                Ebml_SerializeUnsigned(glob, TrackNumber, trackNumber);
                glob->track_id_pos = ftello(glob->stream);
                Ebml_SerializeUnsigned32(glob, TrackUID, trackID);
                Ebml_SerializeUnsigned(glob, TrackType, 1); //video is always 1
                Ebml_SerializeString(glob, CodecID, "V_VP8");
                {
                    unsigned int pixelWidth = cfg->g_w;
                    unsigned int pixelHeight = cfg->g_h;
                    float        frameRate   = (float)fps->num/(float)fps->den;

                    EbmlLoc videoStart;
                    Ebml_StartSubElement(glob, &videoStart, Video);
                    Ebml_SerializeUnsigned(glob, PixelWidth, pixelWidth);
                    Ebml_SerializeUnsigned(glob, PixelHeight, pixelHeight);
                    Ebml_SerializeUnsigned(glob, StereoMode, stereo_fmt);
                    Ebml_SerializeFloat(glob, FrameRate, frameRate);
                    Ebml_EndSubElement(glob, &videoStart); //Video
                }
                Ebml_EndSubElement(glob, &start); //Track Entry
            }
            Ebml_EndSubElement(glob, &trackStart);
        }
        // segment element is open
    }
}


static void
write_webm_block(EbmlGlobal                *glob,
                 const vpx_codec_enc_cfg_t *cfg,
                 const vpx_codec_cx_pkt_t  *pkt)
{
    unsigned long  block_length;
    unsigned char  track_number;
    unsigned short block_timecode = 0;
    unsigned char  flags;
    int64_t        pts_ms;
    int            start_cluster = 0, is_keyframe;

    /* Calculate the PTS of this frame in milliseconds */
    pts_ms = pkt->data.frame.pts * 1000
             * (uint64_t)cfg->g_timebase.num / (uint64_t)cfg->g_timebase.den;
    if(pts_ms <= glob->last_pts_ms)
        pts_ms = glob->last_pts_ms + 1;
    glob->last_pts_ms = pts_ms;

    /* Calculate the relative time of this block */
    if(pts_ms - glob->cluster_timecode > SHRT_MAX)
        start_cluster = 1;
    else
        block_timecode = pts_ms - glob->cluster_timecode;

    is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY);
    if(start_cluster || is_keyframe)
    {
        if(glob->cluster_open)
            Ebml_EndSubElement(glob, &glob->startCluster);

        /* Open the new cluster */
        block_timecode = 0;
        glob->cluster_open = 1;
        glob->cluster_timecode = pts_ms;
        glob->cluster_pos = ftello(glob->stream);
        Ebml_StartSubElement(glob, &glob->startCluster, Cluster); //cluster
        Ebml_SerializeUnsigned(glob, Timecode, glob->cluster_timecode);

        /* Save a cue point if this is a keyframe. */
        if(is_keyframe)
        {
            struct cue_entry *cue, *new_cue_list;
			
            new_cue_list = (cue_entry*)realloc(glob->cue_list,
                                   (glob->cues+1) * sizeof(struct cue_entry));
            if(new_cue_list)
                glob->cue_list = new_cue_list;
            else
                return;//fatal("Failed to realloc cue list.");

            cue = &glob->cue_list[glob->cues];
            cue->time = glob->cluster_timecode;
            cue->loc = glob->cluster_pos;
            glob->cues++;
        }
    }

    /* Write the Simple Block */
    Ebml_WriteID(glob, SimpleBlock);

    block_length = pkt->data.frame.sz + 4;
    block_length |= 0x10000000;
    Ebml_Serialize(glob, &block_length, sizeof(block_length), 4);

    track_number = 1;
    track_number |= 0x80;
    Ebml_Write(glob, &track_number, 1);

    Ebml_Serialize(glob, &block_timecode, sizeof(block_timecode), 2);

    flags = 0;
    if(is_keyframe)
        flags |= 0x80;
    if(pkt->data.frame.flags & VPX_FRAME_IS_INVISIBLE)
        flags |= 0x08;
    Ebml_Write(glob, &flags, 1);

    Ebml_Write(glob, pkt->data.frame.buf, pkt->data.frame.sz);
}


static void
write_webm_file_footer(EbmlGlobal *glob, long hash)
{

    if(glob->cluster_open)
        Ebml_EndSubElement(glob, &glob->startCluster);

    {
        EbmlLoc start;
        unsigned int i;

        glob->cue_pos = ftello(glob->stream);
        Ebml_StartSubElement(glob, &start, Cues);
        for(i=0; i<glob->cues; i++)
        {
            struct cue_entry *cue = &glob->cue_list[i];
            EbmlLoc start;

            Ebml_StartSubElement(glob, &start, CuePoint);
            {
                EbmlLoc start;

                Ebml_SerializeUnsigned(glob, CueTime, cue->time);

                Ebml_StartSubElement(glob, &start, CueTrackPositions);
                Ebml_SerializeUnsigned(glob, CueTrack, 1);
                Ebml_SerializeUnsigned64(glob, CueClusterPosition,
                                         cue->loc - glob->position_reference);
                //Ebml_SerializeUnsigned(glob, CueBlockNumber, cue->blockNumber);
                Ebml_EndSubElement(glob, &start);
            }
            Ebml_EndSubElement(glob, &start);
        }
        Ebml_EndSubElement(glob, &start);
    }

    Ebml_EndSubElement(glob, &glob->startSegment);

    /* Patch up the seek info block */
    write_webm_seek_info(glob);

    /* Patch up the track id */
    fseeko(glob->stream, glob->track_id_pos, SEEK_SET);
    Ebml_SerializeUnsigned32(glob, TrackUID, glob->debug ? 0xDEADBEEF : hash);

    fseeko(glob->stream, 0, SEEK_END);
}

void RGB_To_YV12( unsigned char *pRGBData, int nFrameWidth, int nFrameHeight, int nRGBStride, void *pFullYPlane, void *pDownsampledUPlane, void *pDownsampledVPlane )
{
    int nRGBBytes = nFrameWidth * nFrameHeight * 3;

    // Convert RGB -> YV12. We do this in-place to avoid allocating any more memory.
    unsigned char *pYPlaneOut = (unsigned char*)pFullYPlane;
    int nYPlaneOut = 0;

	int x, y;
	for(y=0; y<nFrameHeight;y++)
	{
		for (x=0; x < nFrameWidth; x ++)
		{
			int nRGBOffs = y*nRGBStride+x*3;
			int nYOffs = y*nFrameWidth+x;

			unsigned char B = pRGBData[nRGBOffs+0];
			unsigned char G = pRGBData[nRGBOffs+1];
			unsigned char R = pRGBData[nRGBOffs+2];

			float y = (float)( R*66 + G*129 + B*25 + 128 ) / 256 + 16;
			float u = (float)( R*-38 + G*-74 + B*112 + 128 ) / 256 + 128;
			float v = (float)( R*112 + G*-94 + B*-18 + 128 ) / 256 + 128;

			// NOTE: We're converting pRGBData to YUV in-place here as well as writing out YUV to pFullYPlane/pDownsampledUPlane/pDownsampledVPlane.
			pRGBData[nRGBOffs+0] = (unsigned char)y;
			pRGBData[nRGBOffs+1] = (unsigned char)u;
			pRGBData[nRGBOffs+2] = (unsigned char)v;

			// Write out the Y plane directly here rather than in another loop.
			pYPlaneOut[nYPlaneOut++] = pRGBData[nRGBOffs+0];
		}
	}

    // Downsample to U and V.
    int halfHeight = nFrameHeight >> 1;
    int halfWidth = nFrameWidth >> 1;

    unsigned char *pVPlaneOut = (unsigned char*)pDownsampledVPlane;
    unsigned char *pUPlaneOut = (unsigned char*)pDownsampledUPlane;

    for ( int yPixel=0; yPixel < halfHeight; yPixel++ )
    {
        int iBaseSrc = ( (yPixel*2) * nRGBStride );

        for ( int xPixel=0; xPixel < halfWidth; xPixel++ )
        {
            pVPlaneOut[yPixel * halfWidth + xPixel] = pRGBData[iBaseSrc + 2];
            pUPlaneOut[yPixel * halfWidth + xPixel] = pRGBData[iBaseSrc + 1];

            iBaseSrc += 6;
        }
    }
}

//-----------------------------------------------
// CVideoRecorder impl
//-----------------------------------------------

CVideoRecorder::CVideoRecorder()
{
	m_ScreenshotType = SCREENSHOT_TYPE_VIRTUAL_SCREEN;
	m_nVideoDuration = 60000;
	m_nVideoFrameInterval = 300;
	m_dwProcessId = 0;
	m_nFrameCount = 0;
	m_nFileId = 0;
	m_nFrameId = 0;
	m_nVideoQuality = VPX_DL_REALTIME;
	m_DesiredFrameSize.cx = 0;
	m_DesiredFrameSize.cy = 0;
	m_hbmpFrame = NULL;
	m_pFrameBits = NULL;
	m_pDIB = NULL;
}

CVideoRecorder::~CVideoRecorder()
{
}

BOOL CVideoRecorder::Init(LPCTSTR szSaveToDir, 
			SCREENSHOT_TYPE type,
			DWORD dwProcessId,
			int nVideoDuration,
			int nVideoFrameInterval,
			int nVideoQuality,
			SIZE* pDesiredFrameSize)
{
	m_sSaveToDir = szSaveToDir;
	m_ScreenshotType = type;
	m_nVideoDuration = nVideoDuration;
	m_nVideoFrameInterval = nVideoFrameInterval;
	m_nVideoQuality = nVideoQuality;
	
	// Save desired frame size
	if(pDesiredFrameSize)
		m_DesiredFrameSize = *pDesiredFrameSize;
	else
	{
		m_DesiredFrameSize.cx = 0; // auto
		m_DesiredFrameSize.cy = 0;
	}

	// Calculate max frame count
	m_nFrameCount = m_nVideoDuration/m_nVideoFrameInterval;
	m_nFileId = 0;
	m_nFrameId = 0;

	// Create folder where to save video frames
	CString sDirName = m_sSaveToDir + _T("\\~temp_video");
	if(!Utility::CreateFolder(sDirName))
	{
		// Error creating temp folder
		return FALSE;
	}

	// Done
	return TRUE;
}

BOOL CVideoRecorder::RecordVideoFrame()
{
	// The following method records a single video frame and returns.
		
    ScreenshotInfo ssi; // Screenshot params    
		
	CString sDirName = m_sSaveToDir + _T("\\~temp_video");

	// Take the screen shot and save it as raw BMP file.
	BOOL bTakeScreenshot = m_sc.TakeDesktopScreenshot(		
		sDirName, 
		ssi, m_ScreenshotType, m_dwProcessId, 
		SCREENSHOT_FORMAT_BMP, 0, FALSE, m_nFileId);
	if(bTakeScreenshot==FALSE)
	{
		// Failed to take screenshot
		return FALSE;
	}
				
	// Save video frame info
	SetVideoFrameInfo(m_nFrameId, ssi);

	// Increment file ID
	m_nFileId += ssi.m_aMonitors.size();

	// Increment frame number
	m_nFrameId++;

	// Reuse files cyclically
	if(m_nFrameId>=m_nFrameCount)
	{
		m_nFileId = 0;
		m_nFrameId = 0;					
	}

	return TRUE;
}

void CVideoRecorder::SetVideoFrameInfo(int nFrameId, ScreenshotInfo& ssi)
{
	if((int)m_aVideoFrames.size()<=nFrameId)
	{
		// Add frame to the end of sequence
		m_aVideoFrames.push_back(ssi);
	}
	else
	{
		// Replace existing frame
				
		/*size_t j;
		for(j=0; j<m_aVideoFrames[nFrameId].m_aMonitors.size(); j++)
		{
			Utility::RecycleFile(m_aVideoFrames[nFrameId].m_aMonitors[j].m_sFileName, TRUE);
		}*/
		
		m_aVideoFrames[nFrameId]=ssi;
	}
}

BOOL CVideoRecorder::EncodeVideo()
{
	ATLASSERT(0);

	// This method encodes all raw BMP files
	// into a single WebM file.

	FILE* fout = NULL;
	vpx_codec_ctx_t      codec;
    vpx_codec_enc_cfg_t  cfg;
    int                  frame_cnt = 0;
    vpx_image_t          raw;
    vpx_codec_err_t      res;
    int                  frame_avail;
    int                  got_data;
    int                  flags = 0;
	EbmlGlobal ebml;
	
	// Init embl
	memset(&ebml, 0, sizeof(EbmlGlobal));
	
	/* Determine frame width and height */
	SIZE ScreenSize={0,0};
	size_t i;
	for(i=0; i<m_aVideoFrames.size(); i++)
	{
		ScreenshotInfo& ssi = m_aVideoFrames[i];
		if(ScreenSize.cx<ssi.m_rcVirtualScreen.right ||
			ScreenSize.cy<ssi.m_rcVirtualScreen.bottom)
		{
			ScreenSize.cx = ssi.m_rcVirtualScreen.right;
			ScreenSize.cy = ssi.m_rcVirtualScreen.bottom;
		}
	}

	int nFrameWidth = 0;
	int nFrameHeight = 0;

	if(m_DesiredFrameSize.cx==0 && m_DesiredFrameSize.cy==0)
	{
		// Auto
		nFrameWidth = ScreenSize.cx;
		nFrameHeight = ScreenSize.cy;
	}
	else
	{
		if(m_DesiredFrameSize.cx!=0)
			nFrameWidth=m_DesiredFrameSize.cx;
		if(m_DesiredFrameSize.cy!=0)
			nFrameHeight=m_DesiredFrameSize.cy;
		
		float ratio = (float)ScreenSize.cx/(float)ScreenSize.cy;
		if(m_DesiredFrameSize.cx==0)
		{
			nFrameWidth = nFrameHeight/ratio;
		}
		else if(m_DesiredFrameSize.cy!=0)
		{
			nFrameHeight = nFrameWidth*ratio;
		}
	}

	// Check maxium allowed frame size
	if(nFrameWidth>1024 || nFrameHeight>1024)
	{

	}

	/* Populate encoder configuration */                                      //
    res = vpx_codec_enc_config_default(interface, &cfg, 0);                   //
    if(res) {                                                                 //
        printf("Failed to get config: %s\n", vpx_codec_err_to_string(res));   //
        return EXIT_FAILURE;                                                  //
    }                                                                         //
 
	vpx_rational timebase;
	timebase.num = m_nVideoFrameInterval;
	timebase.den = 1000;
	
    /* Update the default configuration with our settings */                  //
    cfg.rc_target_bitrate = nFrameWidth * nFrameHeight * cfg.rc_target_bitrate         
                            / cfg.g_w / cfg.g_h;                              
    cfg.g_w = nFrameWidth;                                                        
    cfg.g_h = nFrameHeight; 
	cfg.g_timebase = timebase;

	/* Initialize codec */                                                
    if(vpx_codec_enc_init(&codec, interface, &cfg, 0))                    
        return FALSE;     

	/* Allocate image */
	if(!vpx_img_alloc(&raw, VPX_IMG_FMT_YV12, nFrameWidth, nFrameHeight, 1))
		return FALSE;

	/*Open output file */
	CString sFileName = m_sSaveToDir + _T("\\video.webm");
	m_sOutFile = sFileName;
	_tfopen_s(&fout, sFileName, _T("wb"));
	if(fout==NULL)
		return FALSE;

	ebml.stream = fout;
	ebml.cue_list = NULL;
	ebml.cues = 0;

	vpx_rational framerate;
	framerate.num = 1000;//m_nVideoFrameInterval;
	framerate.den = 1000;
	
	/*Write webm file header */ 
	write_webm_file_header(&ebml, &cfg,
                               &framerate,
                               STEREO_FORMAT_MONO);

	/* Encode frames. */
	int nFrame = m_nFrameId; // Start with the oldest frame
	if(nFrame==m_aVideoFrames.size())
		nFrame=0; // Start from the zeroth frame
	for( ; ; )
	{
		/* Compose frame */
		frame_avail = ComposeFrame(nFrame, &raw);
		
		// Encode frame
		if(vpx_codec_encode(&codec, frame_avail? &raw : NULL, frame_cnt,  //
                                1, flags, m_nVideoQuality)) 
		{
			goto cleanup;
		}

		// Read packets
		vpx_codec_iter_t iter = NULL;
        const vpx_codec_cx_pkt_t *pkt = NULL;

		while( (pkt = vpx_codec_get_cx_data(&codec, &iter)) ) 
		{
			// Write packets to webm file
            write_webm_block(&ebml, &cfg, pkt);            
		}

		// Increment total encoded frame count
		if(frame_avail)
			frame_cnt++;
		else 
			break;
				
		// Increment frame index
		nFrame++;
		if(nFrame==m_aVideoFrames.size())
			nFrame=0; // Start from the zeroth frame
	
		if(nFrame==m_nFrameId || frame_cnt>=m_aVideoFrames.size())
			break; // All frames have been encoded
	}

	/* Free codec resources */
	vpx_codec_destroy(&codec);
    
	// Write file footer
	long hash = 0;
	write_webm_file_footer(&ebml, hash);
    free(ebml.cue_list);
    
	// Close file
	fclose(fout);

cleanup:

	if(&raw)
		vpx_img_free(&raw);

	// Delete temp files.
	CString sDirName = m_sSaveToDir + _T("\\~temp_video");
	Utility::RecycleFile(sDirName, true);

	// Done
	return TRUE;
}

BOOL CVideoRecorder::ComposeFrame(int nFrameId, vpx_image_t *pImage)
{
	// This method composes several bitmaps into single frame image

	// Validate input
	if(nFrameId<0 || nFrameId>=m_aVideoFrames.size())
		return FALSE;

	if(pImage==NULL)
		return FALSE;

	if(m_hbmpFrame==NULL)
	{
		// Calculate frame size

		CreateFrameDIB(pImage->w, pImage->h, 24);
	}

	// Walk through monitor bitmaps
	ScreenshotInfo& ssi = m_aVideoFrames[nFrameId];
	size_t i;
	for(i=0; i<ssi.m_aMonitors.size(); i++)
	{
		// Load image from BMP
		CString sFileName = ssi.m_aMonitors[i].m_sFileName;
		HBITMAP hBitmap = LoadBitmapFromBMPFile(sFileName);
		if(hBitmap)
		{
			// Create temp DC
			HDC hMemDC = CreateCompatibleDC(m_hDC);
			// Select loaded bitmap into DC
			HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

			float x_ratio = (float)pImage->w/(float)ssi.m_rcVirtualScreen.Width();
			float y_ratio = (float)pImage->h/(float)ssi.m_rcVirtualScreen.Height();
			int xDest = ssi.m_aMonitors[i].m_rcMonitor.left*x_ratio;
			int yDest = ssi.m_aMonitors[i].m_rcMonitor.top*y_ratio;
			int wDest = ssi.m_aMonitors[i].m_rcMonitor.Width()*x_ratio;
			int hDest = ssi.m_aMonitors[i].m_rcMonitor.Height()*y_ratio;

			// Copy bitmap to its destination rect
			StretchBlt(m_hDC, xDest, yDest, wDest, hDest, hMemDC, 0, 0, 
				ssi.m_aMonitors[i].m_rcMonitor.Width(), ssi.m_aMonitors[i].m_rcMonitor.Height(), SRCCOPY);

			// Select old bitmap into DC
			SelectObject(hMemDC, hOldBitmap);
			// Free DC
			DeleteDC(hMemDC);
			// Free loaded bitmap
			DeleteObject(hBitmap);
		}		
	}

	// Convert RGB to YV12
	RGB_To_YV12((unsigned char*)m_pFrameBits, pImage->w, pImage->h, pImage->w*3+(pImage->w*3)%4, 
		pImage->planes[0], pImage->planes[1], pImage->planes[2]);

	return TRUE;
}

BOOL CVideoRecorder::CreateFrameDIB(DWORD dwWidth, DWORD dwHeight, int nBits)
{
    if (m_pDIB) 
		return FALSE;

    const DWORD dwcBihSize = sizeof(BITMAPINFOHEADER);

    // Calculate the memory required for the DIB
    DWORD dwSize = dwcBihSize +
                    (2>>nBits) * sizeof(RGBQUAD) +
                    ((nBits * dwWidth) * dwHeight);

    m_pDIB = (LPBITMAPINFO)new BYTE[dwSize];
    if (!m_pDIB) 
		return FALSE;


    m_pDIB->bmiHeader.biSize = dwcBihSize;
    m_pDIB->bmiHeader.biWidth = dwWidth;
    m_pDIB->bmiHeader.biHeight = -dwHeight;
    m_pDIB->bmiHeader.biBitCount = nBits;
    m_pDIB->bmiHeader.biPlanes = 1;
    m_pDIB->bmiHeader.biCompression = BI_RGB;
    m_pDIB->bmiHeader.biXPelsPerMeter = 1000;
    m_pDIB->bmiHeader.biYPelsPerMeter = 1000;
    m_pDIB->bmiHeader.biClrUsed = 0;
    m_pDIB->bmiHeader.biClrImportant = 0;

    LPRGBQUAD lpColors =
        (LPRGBQUAD)(m_pDIB+m_pDIB->bmiHeader.biSize);
	int nColors=2>>m_pDIB->bmiHeader.biBitCount;
    for(int i=0;i<nColors;i++)
    {
        lpColors[i].rgbRed=0;
        lpColors[i].rgbBlue=0;
        lpColors[i].rgbGreen=0;
        lpColors[i].rgbReserved=0;
    }

	m_hDC = CreateCompatibleDC(GetDC(NULL));

	m_hbmpFrame = CreateDIBSection(m_hDC, m_pDIB, DIB_RGB_COLORS, &m_pFrameBits,
		NULL, 0);

	m_hOldBitmap = (HBITMAP)SelectObject(m_hDC, m_hbmpFrame);

    return TRUE;
}

HBITMAP CVideoRecorder::LoadBitmapFromBMPFile(LPCTSTR szFileName)
{
	// This method loads a BMP file.
	
	// Use LoadImage() to get the image loaded into a DIBSection
    HBITMAP hBitmap = (HBITMAP)LoadImage( NULL, szFileName, IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );
	if(hBitmap==NULL)
	{
		DWORD dwErr = GetLastError();
		int x=0;
	}
    return hBitmap;
}

CString CVideoRecorder::GetOutFile()
{
	return m_sOutFile;
}