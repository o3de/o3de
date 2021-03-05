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

#include <AzCore/std/string/conversions.h>
#include "VideoPlayback_precompiled.h"

#include "Decoder.h"

#if AZ_TRAIT_VIDEOPLAYBACK_ENABLE_DECODER
#include <AzCore/std/functional.h>
#include <VideoPlaybackFramework/VideoPlaybackBus.h>

// Disable warnings so that this works with FFMPEG and libAV
// due to differing types and deprecated functions.
AZ_PUSH_DISABLE_WARNING(4244 4996, "-Wunknown-warning-option")

namespace AZ
{
    namespace VideoPlayback
    {
        Decoder::Decoder()
        {
        }

        Decoder::~Decoder()
        {
            DeInit();
        }

        bool Decoder::Init()
        {
            av_register_all();

            m_codec = nullptr;
            m_codecContext = nullptr;

            m_formatContext = nullptr;
            m_tempFrame = nullptr;

            m_streamIndex = 0;

            m_swsContext = nullptr;

            return true;
        }

        void Decoder::DeInit()
        {
        }

        void PrintDecoderError(const AZStd::string& message, int errorCode)
        {
            const int bufferLength = 1024;
            char errorMessage[bufferLength];
            int valid = av_strerror(errorCode, errorMessage, bufferLength);
            AZ_Assert(valid == 0, "Error buffer is too small");
            AZ_Printf("VideoPlayback", "%s, error: %s", message.c_str(), errorMessage);
        }

        bool Decoder::LoadVideo(const AZStd::string& filename, AZ::u32 queueAheadCount)
        {
            int errorCode = avformat_open_input(&m_formatContext, filename.c_str(), nullptr, nullptr);
            if (errorCode != 0)
            {
                AZStd::string error = "Unable to open video file " + filename;
                PrintDecoderError(error, errorCode);
                return false;
            }

            errorCode = avformat_find_stream_info(m_formatContext, nullptr);
            if (errorCode < 0)
            {
                AZStd::string error = "Video " + filename + " has no valid streams";
                PrintDecoderError(error, errorCode);
                return false;
            }

            // Locate the proper stream within the video.
            m_codecContext = nullptr;
            m_streamIndex = 0;
            m_avgSecondsPerFrame = 0;
            m_currentTime = 0;

            for (uint32 index = 0; index < m_formatContext->nb_streams; ++index)
            {
                AVStream* stream = m_formatContext->streams[index];

                if (stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
                {
                    m_streamTimeBase = stream->time_base.den;
                    m_durationTimeBase = m_streamTimeBase;
                    m_totalDuration = static_cast<float>(stream->duration) / static_cast<float>(m_durationTimeBase);

                    //Some formats may not report the duration properly in the stream; check the format context if the duration is invalid
                    if (m_totalDuration <= 0)
                    {
                        m_durationTimeBase = AV_TIME_BASE;
                        m_totalDuration = static_cast<float>(m_formatContext->duration) / static_cast<float>(m_durationTimeBase);
                    }

                    m_codecContext = stream->codec;
                    m_streamIndex = index;

                    //Capture average framerate for playback settings
                    //Try to read framerate from stream; not valid for all video types
                    if (stream->avg_frame_rate.den > 0 && stream->avg_frame_rate.num > 0)
                    {
                        m_avgSecondsPerFrame = 1.0f / av_q2d(stream->avg_frame_rate);   
                    }
                    //If the framerate is invalid, we'll try to calculate it from the duration and the frame count
                    else if (stream->nb_frames > 0)
                    {
                        m_avgSecondsPerFrame = m_totalDuration / stream->nb_frames;
                    }

                    m_totalFrameCount = stream->nb_frames;

                    //Try to read stereo mode from side data; not 100% accurate as not all videos report stereo layout
                    for (int i = 0; i < stream->nb_side_data; i++)
                    {
                        AVPacketSideData sideData = stream->side_data[i];

                        if (sideData.type == AV_PKT_DATA_STEREO3D)
                        {
                            AVStereo3D* stereo = reinterpret_cast<AVStereo3D*>(sideData.data);

                            switch (stereo->type)
                            {
                            case AV_STEREO3D_TOPBOTTOM:
                                //If it's not inverted
                                if (!(stereo->flags & AV_STEREO3D_FLAG_INVERT))
                                {
                                    m_stereoLayout = AZ::VR::StereoLayout::TOP_BOTTOM;
                                }
                                else 
                                {
                                    m_stereoLayout = AZ::VR::StereoLayout::BOTTOM_TOP;
                                }
                                break;
                            default:
                                //This will also be reached if the stereo layout is interlaced or some other format that we don't support
                                m_stereoLayout = AZ::VR::StereoLayout::UNKNOWN;
                                break;
                            }
                        }
                    }

                    break;
                }
            }

            if (m_codecContext == nullptr)
            {
                AZStd::string error = "Unable to locate a suitable video codec for " + filename;
                PrintDecoderError(error, errorCode);
                return false;
            }

            m_codec = avcodec_find_decoder(m_codecContext->codec_id);
            if (m_codec == nullptr)
            {
                AZStd::string error = "Unsupported video codec for " + filename;
                AZ_Printf("Decoder", error.c_str());
                return false;
            }

            errorCode = avcodec_open2(m_codecContext, m_codec, nullptr);
            if (errorCode != 0)
            {
                AZStd::string error = "Could not open codec for " + filename;
                PrintDecoderError(error, errorCode);
                return false;
            }

            // Allocate the frame data to read into and get ready for video streaming.
            m_tempFrame = av_frame_alloc();

            AZ::u32 bytesPerFrame = avpicture_get_size(AV_PIX_FMT_RGBA, m_codecContext->width, m_codecContext->height);

            m_RGBAFrames.resize(queueAheadCount);
            m_decodedFrames = new AZStd::atomic_bool[queueAheadCount];
            for (AZ::u32 frame = 0; frame < queueAheadCount; ++frame)
            {
                m_RGBAFrames[frame] = av_frame_alloc();
                m_RGBAFrames[frame]->linesize[0] = m_codecContext->width * 4; // RGBA
                m_RGBAFrames[frame]->data[0] = static_cast<uint8*>(av_malloc(bytesPerFrame));

                m_decodedFrames[frame] = false;
            }

            //Setup this context to rescale frames from whatever pix_fmt is to RGBA
            m_swsContext = sws_getContext(m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
                m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGBA,
                SWS_BILINEAR, nullptr, nullptr, nullptr);

            //Frame 0 isn't always the start of the video; seek to what the file reports as the start
            //This will fire up the decoder thread
            Seek(m_formatContext->start_time);

            return true;
        }

        void Decoder::UnloadVideo()
        {
            KillDecoderThread();

            if (m_codecContext != nullptr)
            {
                avcodec_close(m_codecContext);
            }
            if (m_formatContext != nullptr)
            {
                avformat_close_input(&m_formatContext);
            }
            if (m_tempFrame != nullptr)
            {
                av_frame_free(&m_tempFrame);
            }

            if (m_RGBAFrames.size() > 0)
            {
                for (int frame = 0; frame < m_RGBAFrames.size(); ++frame)
                {
                    if (m_RGBAFrames[frame])
                    {
                        av_free(m_RGBAFrames[frame]->data[0]);
                        av_frame_free(&m_RGBAFrames[frame]);

                        m_RGBAFrames[frame] = nullptr;
                    }
                }
            }
            SAFE_DELETE_ARRAY(m_decodedFrames);

            m_decodedFrames = nullptr;
            m_formatContext = nullptr;
            m_codec = nullptr;
            m_codecContext = nullptr;
            m_tempFrame = nullptr;
            m_streamIndex = 0;
        }

        bool Decoder::GetFrameAhead(FrameInfo& frame, AZ::u64 frameAheadIndex)
        {            
            if (m_formatContext)
            {
                /*Determine the index of the frame to return
                If we've read 25 frames and we wanted to jump ahead 2 frames to read frame 27, 
                that frame will be stored at index 7.
                */
                const AZ::u64 frameIndex = (m_framePresentIndex + (frameAheadIndex - 1) )% m_RGBAFrames.size();
                if (m_decodedFrames[frameIndex])
                {
                    //m_RGBAFrames is preallocated; finalFrame should never be nullptr
                    AVFrame* finalFrame = m_RGBAFrames[frameIndex];

                    frame.data = finalFrame->data[0];
                    frame.width = m_codecContext->width;
                    frame.height = m_codecContext->height;

                    //Determine the presentation timestamp of the frame
                    int64_t pts = 0;
                    if (finalFrame->pts == AV_NOPTS_VALUE)
                    {
                        pts = finalFrame->pkt_dts;
                    }
                    else
                    {
                        pts = finalFrame->pts;
                    }

                    /* Advance frame counter
                    Following the same example, we've displayed 27 frames
                    */
                    m_framePresentIndex += frameAheadIndex;

                    //Calculate the current time in seconds as a float. 
                    m_currentTime = static_cast<float>(pts) / static_cast<float>(m_streamTimeBase);

                    //Save pts as a float in seconds
                    frame.pts = m_currentTime;

                    //Save these so the decoder thread can properly mark the frames that we jumped over as ready for write
                    m_frameClearStartIndex = m_framePresentIndex;
                    m_frameClearCount = frameAheadIndex;

                    SignalDecoderThread(frameAheadIndex);

                    return true;
                }
            }

            return false;
        }

        void Decoder::Presented()
        {
            /* Mark all frames that we passed as ready for writing
            Using the same example as above. If we jumped from frame 25 to frame 27, we need to mark
            frames 26 and 27 as ready for writing because we have passed them.
            */
            for (uint32_t i = 0; i < m_frameClearCount; ++i)
            {
                AZ::u64 frameIndexCounter = (m_frameClearStartIndex + i) % m_RGBAFrames.size();
                m_decodedFrames[frameIndexCounter] = false;
            }
        }

        void Decoder::SignalDecoderThread(AZ::u32 count)
        {
            m_semaphore.release(count);
        }

        void Decoder::DecodeVideoFrame()
        {
            do
            {
                if (m_killThread)
                {
                    return;
                }

                const uint64_t frameIndex = GetFrameIndexToWrite();

                //If the frame we're trying to write to has not been read from yet, just skip it
                if (m_decodedFrames[frameIndex] != false)
                {
                    continue;
                }

                // Decode a frame.
                if (m_formatContext)
                {
                    AVFrame* destFrame = m_RGBAFrames[frameIndex];
                    int frameFinished = 0;
                    AVPacket packet;
                    int returnCode = 0;

                    //If av_read_frame returns anything less than 0 it could either be an error, or the video could have stopped
                    while ((returnCode = av_read_frame(m_formatContext, &packet)) >= 0)
                    {
                        // Is this the video stream we found earlier?
                        if (packet.stream_index == m_streamIndex)
                        {
                            int decodeError = avcodec_decode_video2(m_codecContext, m_tempFrame, &frameFinished, &packet);
                            if (decodeError < 0)
                            {
                                AZStd::string error = "Error while decoding video";
                                PrintDecoderError(error, decodeError);
                            }

                            if (frameFinished)
                            {
                                // Convert the image to RGBA.
                                sws_scale(m_swsContext, m_tempFrame->data, m_tempFrame->linesize, 0, m_codecContext->height, destFrame->data, destFrame->linesize);

                                //Some formats will automatically parse the presentation time stamp from the packet into the frame
                                //Some don't, so we do it manually just in case
                                destFrame->pts = packet.pts;

                                av_free_packet(&packet);

                                break;
                            }
                        }

                        av_free_packet(&packet);
                    }

                    //Write a log if the return code was not simply End of File
                    if (returnCode < 0 && returnCode != AVERROR_EOF)
                    {
                        AZStd::string error = "Error occurred when decoding video ";
                        PrintDecoderError(error, returnCode);
                    }

                    //Mark when we've hit the end of the file; video isn't "finished" until EOF and no frames left to process
                    if ((returnCode == AVERROR_EOF))
                    {
                        m_endOfFile = true;
                    }

                    //Mark the decoded frame as ready to present
                    m_decodedFrames[frameIndex] = true;
                    ++m_frameDecodeIndex;
                }

                // Wait for a semaphore to kick off decoding of a frame.
                m_semaphore.acquire();

            } while (true);
        }

        const uint64_t Decoder::GetFrameIndexToWrite()
        {
            return m_frameDecodeIndex % m_RGBAFrames.size();
        }

        void Decoder::KillDecoderThread() 
        {
            //Kill decoder thread
            m_killThread = true;
            SignalDecoderThread(1); // just in case.
            if (m_decoderThread.joinable())
            {
                m_decoderThread.join();
            }
        }
        void Decoder::StartDecoderThread() 
        {
            m_killThread = false;
            m_frameDecodeIndex = 0;
            m_framePresentIndex = 0;
            AZStd::function<void()> threadFunction = AZStd::bind(&Decoder::DecodeVideoFrame, this);
            AZStd::thread_desc threadDesc;
            m_decoderThread = AZStd::thread(threadFunction, &threadDesc);
        }

        void Decoder::GetVideoInfo(AZ::u32& width, AZ::u32& height, float& secondsPerFrame)
        {
            if (m_codecContext)
            {
                width = m_codecContext->width;
                height = m_codecContext->height;
                secondsPerFrame = m_avgSecondsPerFrame;
            }
        }

        void Decoder::Seek(AZ::s64 timestamp)
        {
            KillDecoderThread();

            m_currentTime = timestamp;

            //Mark all frames as ready for write
            for (size_t frame = 0; frame < m_RGBAFrames.size(); ++frame)
            {
                m_decodedFrames[frame] = false;
            }

            //Seek to the timestamp
            AZ::s32 flags = AVSEEK_FLAG_BACKWARD;
            if (timestamp > 0)
            {
                flags |= AVSEEK_FLAG_ANY;
            }

            AZ::s32 error = avformat_seek_file(m_formatContext, m_streamIndex, timestamp, timestamp, timestamp, flags);
            //If seeking fails, try seeking JUST with AVSEEK_FLAG_ANY
            if (error < 0)
            {
                error = av_seek_frame(m_formatContext, m_streamIndex, timestamp, AVSEEK_FLAG_ANY);
            }

            if (error < 0)
            {
                AZStd::string errorMsg = "Unable to seek to timestamp: " + AZStd::to_string(timestamp);
                PrintDecoderError(errorMsg, error);
            }

            avcodec_flush_buffers(m_codecContext);

            //Restart decoding
            StartDecoderThread();

            //Signal the decoder thread to start up again
            SignalDecoderThread(m_RGBAFrames.size());

            m_endOfFile = false;
        }

        bool Decoder::IsFinished()
        {
            bool finishedPlayback = false;

            //If the container provided us with the number of frames, use that to determine if playback was completed
            if (m_totalFrameCount > 0)
            {
                if (m_framePresentIndex >= m_totalFrameCount)
                {
                    finishedPlayback = true;
                }
            }
            else
            {
                //If the current time is within 80ms of the total duration
                //This is a bit hack but there's no good, universal way to determine if we've read the last frame
                //Durations in FFmpeg/LibAV are not exact; often they're estimated from other data provided by the codec
                if (m_currentTime + 0.08f >= m_totalDuration)
                {
                    finishedPlayback = true;
                }
            }

            return m_endOfFile && finishedPlayback;
        }
    } //namespace VideoPlayback
}//namespace AZ

AZ_POP_DISABLE_WARNING

#endif
