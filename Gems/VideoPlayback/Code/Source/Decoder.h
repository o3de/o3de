/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

/*
    This decoder provides an interface for loading video files, decoding them and 
    providing RGB texture data to lumberyard. 
*/

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/semaphore.h>
#include <VideoPlayback_Traits_Platform.h>

#if AZ_TRAIT_VIDEOPLAYBACK_ENABLE_DECODER
extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/stereo3d.h>
}

#include <VRCommon.h>

namespace AZ
{
    namespace VideoPlayback
    {

        class Decoder
        {
        public:

            Decoder();
            ~Decoder();

            /*
            * Initialize the FFPMEG interface
            */
            bool Init();

            /*
            * Shut down the FFMPEG interface
            */
            void DeInit();

            /*
            * Load the video off the disk with FFMPEG
            *
            * @param filename The absolute path to the video file
            * @param queuAheadCount How many frames ahead the decoder should try to be when decoding this video
            *
            * @return True if the video loaded successfully
            */
            bool LoadVideo(const AZStd::string& filename, AZ::u32 queueAheadCount);

            /*
            * Stops playback and unloads the video from memory
            */
            void UnloadVideo();

            struct FrameInfo
            {
                const AZ::u8* data = nullptr;
                AZ::u32 width = 0;
                AZ::u32 height = 0;
                float pts;
            };

            /*
            * Retrieves a frame 'x' number frames ahead of where the decoder currently is
            *
            * @param frame A reference to the FrameInfo struct to be filled
            * @param frameAheadIndex How many frames ahead from the current frame we should read
            *
            * @return True if the frame was retrieved
            */
            bool GetFrameAhead(FrameInfo& frame, AZ::u64 frameAheadIndex);

            /* Call this after the frame returned by GetFrameAhead has actually been 
            * presented and is no longer needed so the decoder can write over it
            * without causing visual artifacts.
            */
            void Presented();

            /*
            * Retrieves basic info about the video
            *
            * @param width The width of the video
            * @param height The height of the video
            * @param secondsPerFrame How many seconds pass per frame of the video
            */
            void GetVideoInfo(AZ::u32& width, AZ::u32& height, float& secondsPerFrame);

            /*
            * Seeks to a timestamp in the movie
            *
            * @param timestamp The timestamp to seek to in the movie (in seconds)
            */
            void Seek(AZ::s64 timestamp);

            /* 
            * Returns whether or not the decoder has processed all frames in the video
            *
            * @return True if the decoder has finished processing all frames in the video
            */
            bool IsFinished();

            /* Returns the stereo layout for this video
            * If the video has no spherical metadata the type will be UNKNOWN.
            * This doesn't mean the video is not stereo! It simply means that the
            * layout was not stored in the video's metadata OR that the stereo layout
            * is something unsupported (like interlaced stereo). 
            */
            inline const AZ::VR::StereoLayout& GetStereoLayout() const
            {
                return m_stereoLayout;
            }

        private:

            /* Signals m_semaphore to allow the decoding thread to decode a given number of frames 
            * @param count How many times we want to signal the semaphore and therefore 
            * how many frames we want to tell the decode thread to decode next. 
            */
            void SignalDecoderThread(AZ::u32 count);

            /* Threaded method that handles decoding video frames
            */
            void DecodeVideoFrame();

            /* Returns the index of the frame to write to next.
            */
            const AZ::u64 GetFrameIndexToWrite();

            void KillDecoderThread();
            void StartDecoderThread();
            
            AVCodec* m_codec = nullptr;
            AVCodecContext* m_codecContext = nullptr;
            AVFormatContext* m_formatContext = nullptr;

            AZ::u32 m_frameClearStartIndex = 0;
            AZ::u32 m_frameClearCount = 0;

            AVFrame* m_tempFrame = nullptr;             //A temporary frame that acts as a buffer to avoid writing garbage frames directly into m_RGBAFrames
            AZStd::vector<AVFrame*> m_RGBAFrames;       //A collection of frame data that is accessed like a ring buffer. 

            AZ::u64 m_totalFrameCount;

            float m_totalDuration;  //The total time in seconds that this video lasts
            float m_currentTime;    //The timestamp of the last frame that was presented

            AZ::u32 m_streamIndex = 0;      //The index of the target video stream. Streams collection found at m_formatContext->streams
            AZ::u32 m_streamTimeBase = 0;   //Base time factor used in the duration reported from the stream
            AZ::u32 m_durationTimeBase = 0; //Base time factor used in the duration reported from m_formatContext
            float m_avgSecondsPerFrame = 0; 

            SwsContext *m_swsContext = nullptr; //Used for rescaling and reorganizing frames. Used in this class to convert video frames to RGBA from whatever format they're stored in

            AZStd::thread m_decoderThread;
            AZStd::atomic_bool m_killThread;
            AZ::u64 m_frameDecodeIndex;         //An index marking which frame in m_RGBAFrames should be decoded next
            AZ::u64 m_framePresentIndex;        //An index marking which frame in m_RGBAFrames should be presented next
            AZStd::semaphore m_semaphore;

            //Denotes which frames in m_RGBAFrames are ready for read / write
            AZStd::atomic_bool* m_decodedFrames = nullptr;  //The size of this array is the same size as m_RGBAFrames. 

            //Set to true when the decoder has hit the EOF marker
            bool m_endOfFile = false;

            AZ::s64 m_seekToTimestamp = -1;

            AZ::VR::StereoLayout m_stereoLayout = AZ::VR::StereoLayout::UNKNOWN;
        };
    } // namespace VideoPlayback
}//namespace AZ

#endif
