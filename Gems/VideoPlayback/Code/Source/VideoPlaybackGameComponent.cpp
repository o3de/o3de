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
#include "VideoPlayback_precompiled.h"

#include "VideoPlaybackGameComponent.h"
#include <IRenderer.h>
#include <IStereoRenderer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Casting/numeric_cast.h>
#include <VideoPlayback_Traits_Platform.h>

#if AZ_TRAIT_VIDEOPLAYBACK_ENABLE_DECODER

namespace AZ
{
    namespace VideoPlayback
    {
        void VideoPlaybackGameComponent::Init()
        {
            bool success = m_videoDecoder.Init();
            AZ_Assert(success, "Unable to initialized video decoder");
        }

        void VideoPlaybackGameComponent::Activate()
        {
            const AZStd::string& assetPath = m_videoAsset.GetAssetPath();
            if (assetPath.length())
            {
                const AZ::u64 pathSize = 1024;
                char resolvedPath[pathSize];
                AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(assetPath.c_str(), resolvedPath, pathSize);

                m_queueAheadCount = (m_queueAheadCount < 1) ? 1 : m_queueAheadCount; // At least 1 frame ahead.
                if (m_videoDecoder.LoadVideo(resolvedPath, m_queueAheadCount))
                {
                    // Create the render texture to display the video with.
                    AZ::u32 videoWidth = 0;
                    AZ::u32 videoHeight = 0;
                    m_videoDecoder.GetVideoInfo(videoWidth, videoHeight, m_secondsPerFrame);

                    //Check that the video info is valid
                    if (videoWidth <= 0 || videoHeight <= 0 || m_secondsPerFrame <= 0)
                    {
                        return;
                    }

                    //If a texture name ends in _stereo we want to create a stereo texture
                    const AZStd::string ending = "_stereo";
                    size_t nameLength = m_userTextureName.length();
                    size_t endingLength = ending.length();
                    m_isStereo = (nameLength > endingLength && m_userTextureName.compare(nameLength - endingLength, endingLength, ending) == 0);

                    //If the layout is "UNKNOWN" we're going to try to use the reported layout from the decoder
                    if (m_preferredStereoLayout == AZ::VR::StereoLayout::UNKNOWN)
                    {
                        const AZ::VR::StereoLayout& reportedLayout = m_videoDecoder.GetStereoLayout();
                        //If the decoder could determine the video's preferred layout, lets use that
                        if (reportedLayout != AZ::VR::StereoLayout::UNKNOWN)
                        {
                            m_preferredStereoLayout = reportedLayout;
                        }
                    }

                    if (m_isStereo)
                    {
                        //For stereo video create two textures; one for each eye
                        AZStd::string leftName = m_userTextureName + "_Left";
                        AZStd::string rightName = m_userTextureName + "_Right";

                        //Cut the stereo textures in half
                        if (m_preferredStereoLayout == AZ::VR::StereoLayout::TOP_BOTTOM || m_preferredStereoLayout == AZ::VR::StereoLayout::BOTTOM_TOP)
                        {
                            videoHeight = videoHeight >> 1;
                        }

                        //If the stereo type is "UNKNOWN" but a stereo texture was requested, we're just going to show the same full texture in each eye
                        //That way it's obvious that the stereo type was not automatically determined and we won't end up slicing up a possibly non-stereo video
                        //and hurting someone's eyes.

                        m_videoTextureLeft = gEnv->pRenderer->DownLoadToVideoMemory(nullptr, videoWidth, videoHeight, eTF_R8G8B8A8, eTF_R8G8B8A8, 1, false, FILTER_BILINEAR, 0, leftName.c_str());
                        m_videoTextureRight = gEnv->pRenderer->DownLoadToVideoMemory(nullptr, videoWidth, videoHeight, eTF_R8G8B8A8, eTF_R8G8B8A8, 1, false, FILTER_BILINEAR, 0, rightName.c_str());
                    }
                    else
                    {
                        //Otherwise just create one texture and store it in the "Left" texture ID variable to save space
                        m_videoTextureLeft = gEnv->pRenderer->DownLoadToVideoMemory(nullptr, videoWidth, videoHeight, eTF_R8G8B8A8, eTF_R8G8B8A8, 1, false, FILTER_BILINEAR, 0, m_userTextureName.c_str());
                    }

                    //Only connect to buses if the video we've loaded is valid
                    m_entityId = GetEntityId();

                    VideoPlaybackFramework::VideoPlaybackRequestBus::Handler::BusConnect(m_entityId);
                    AZ::TickBus::Handler::BusConnect();
                }
            }
        }

        void VideoPlaybackGameComponent::Deactivate()
        {
            m_playing = false;
            m_secondsSinceLastFrame = 0.0f;
            m_videoDecoder.UnloadVideo();

            AZ::TickBus::Handler::BusDisconnect();
            VideoPlaybackFramework::VideoPlaybackRequestBus::Handler::BusDisconnect();

            if (gEnv && gEnv->pRenderer)
            {
                gEnv->pRenderer->RemoveTexture(m_videoTextureLeft);
                gEnv->pRenderer->RemoveTexture(m_videoTextureRight);
            }
            m_videoTextureLeft = 0;
            m_videoTextureRight = 0;
        }

        void VideoPlaybackGameComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<VideoPlaybackGameComponent, AZ::Component>()
                    ->Version(0)
                    ->Field("Video", &VideoPlaybackGameComponent::m_videoAsset)
                    ->Field("Texture name", &VideoPlaybackGameComponent::m_userTextureName)
                    ->Field("Stereo layout", &VideoPlaybackGameComponent::m_preferredStereoLayout)
                    ->Field("Queue ahead count", &VideoPlaybackGameComponent::m_queueAheadCount)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    auto editInfo = editContext->Class<VideoPlaybackGameComponent>("Video Playback", "Component to handle playing a video.");
                    editInfo->DataElement(AZ::Edit::UIHandlers::Default, &VideoPlaybackGameComponent::m_videoAsset, "Video", "Video to play.")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &VideoPlaybackGameComponent::m_userTextureName, "Texture name", "User-named texture to use on the material for this entity")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VideoPlaybackGameComponent::OnRenderTextureChange)
                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &VideoPlaybackGameComponent::m_preferredStereoLayout, "Stereo layout", "How the video is laid out for stereo playback")
                            ->EnumAttribute(AZ::VR::StereoLayout::UNKNOWN, "Auto-detect")
                            ->EnumAttribute(AZ::VR::StereoLayout::TOP_BOTTOM, "Top-Bottom")
                            ->EnumAttribute(AZ::VR::StereoLayout::BOTTOM_TOP, "Bottom-Top")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &VideoPlaybackGameComponent::m_queueAheadCount, "Frame queue ahead count", "How many frames ahead to buffer the video")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/VideoPlayback.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void VideoPlaybackGameComponent::Play()
        {
            if (m_playing == false)
            {
                m_playing = true;
                m_secondsSinceLastFrame = 0.0f;
                EBUS_EVENT_ID(m_entityId, VideoPlaybackFramework::VideoPlaybackNotificationBus, OnPlaybackStarted);
            }
        }

        void VideoPlaybackGameComponent::Pause()
        {
            if (m_playing == true)
            {
                m_playing = false;
                m_secondsSinceLastFrame = 0.0f;
                EBUS_EVENT_ID(m_entityId, VideoPlaybackFramework::VideoPlaybackNotificationBus, OnPlaybackPaused);
            }
        }

        void VideoPlaybackGameComponent::Stop()
        {
            m_shouldStop = true;
            m_playing = true;       //Set playing to true so that we can keep "playing" and seek to the beginning of the movie
            EBUS_EVENT_ID(m_entityId, VideoPlaybackFramework::VideoPlaybackNotificationBus, OnPlaybackStopped);
        }

        bool VideoPlaybackGameComponent::IsPlaying()
        {
            return m_playing;
        }

        AZ::u32 VideoPlaybackGameComponent::GetQueueAheadCount()
        {
            return m_queueAheadCount;
        }

        void VideoPlaybackGameComponent::SetQueueAheadCount(AZ::u32 queueAheadCount)
        {
            m_queueAheadCount = queueAheadCount;
        }

        bool VideoPlaybackGameComponent::GetIsLooping()
        {
            return m_shouldLoop;
        }

        void VideoPlaybackGameComponent::SetIsLooping(bool isLooping)
        {
            m_shouldLoop = isLooping;
        }

        bool VideoPlaybackGameComponent::GetIsAutoPlay()
        {
            return false;
        }

        void VideoPlaybackGameComponent::SetIsAutoPlay([[maybe_unused]] bool isAutoPlay)
        {
        }

        float VideoPlaybackGameComponent::GetPlaybackSpeed()
        {
            return 1.0f / m_playbackSpeedFactor;
        }

        void VideoPlaybackGameComponent::SetPlaybackSpeed(float speedFactor)
        {
            //Allow the speedFactor to approach zero but never cross it
            if (speedFactor <= 0)
            {
                AZ_Warning("VideoPlayback", false, "Speed Factor %f cannot be less than or equal to 0. Default playback speed used instead to avoid error.", speedFactor);
                speedFactor = 1;
            }

            //We actually want the playback speed factor to be inverted since we use it to modify
            //m_secondsPerFrame. To go at half speed we pass in 0.5f but we want the secondsPerFrame to double
            //so m_playbackSpeedFactor will have to be 2.0f.
            m_playbackSpeedFactor = 1.0f / speedFactor;
        }

        AZStd::string VideoPlaybackGameComponent::GetVideoPathname() const
        {
            return m_videoAsset.GetAssetPath();
        }

        void VideoPlaybackGameComponent::SetVideoPathname(const AZStd::string& videoPath)
        {
            m_videoAsset.SetAssetPath(videoPath.c_str());
        }

        AZStd::string VideoPlaybackGameComponent::GetDestinationTextureName() const
        {
            return m_userTextureName;
        }

        void VideoPlaybackGameComponent::SetDestinationTextureName(const AZStd::string& destinationTextureName)
        {
            m_userTextureName = destinationTextureName;
            OnRenderTextureChange();
        }

        void VideoPlaybackGameComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            //If we've received a signal to stop, the best way to do that
            //is to stop playing and seek back to the earliest possible timestamp.
            //Timestamp 0 does not always mean the first frame or the beginning of the video for every format
            //Seeking to the smallest possible 64 signed integer is a good way to guarantee that you'll 
            //go back to the first frame.
            if (m_shouldStop == true)
            {
                m_videoDecoder.Seek(INT64_MIN);
                m_shouldStop = false;
                m_playing = false;
                m_secondsSinceLastFrame = 0;
                return;
            }

            if (m_playing)
            {
                m_secondsSinceLastFrame += deltaTime;
                float playbackSpeed = m_secondsPerFrame * m_playbackSpeedFactor;
                if (playbackSpeed > 0 && m_secondsSinceLastFrame >= playbackSpeed)
                {
                    //Determine how many frames ahead we need to go
                    //If it's been 0.3 seconds but we want to play a frame every 0.1 seconds we need to move forward 3 frames. 
                    uint32_t framesAhead = aznumeric_caster(floor(m_secondsSinceLastFrame / playbackSpeed)); //Round up

                    //We can't jump more frames ahead than we have queued up
                    if (framesAhead > m_queueAheadCount)
                    {
                        framesAhead = m_queueAheadCount;
                    }

                    if (m_videoDecoder.GetFrameAhead(m_frame, framesAhead))
                    {
                        int frameWidth = m_frame.width;
                        int frameHeight = m_frame.height;

                        const uint8* leftFrameData = m_frame.data;    //For when there is no stereo
                        const uint8* rightFrameData = nullptr;

                        //If we're in stereo mode, cut up the height and assign data offsets here
                        if (m_isStereo)
                        {
                            uint32_t halfFrameOffset = 0;

                            if (m_preferredStereoLayout == AZ::VR::StereoLayout::TOP_BOTTOM || m_preferredStereoLayout == AZ::VR::StereoLayout::BOTTOM_TOP)
                            {
                                frameHeight = frameHeight >> 1;
                                halfFrameOffset = (frameHeight * m_frame.width) * 4;

                                if (m_preferredStereoLayout == AZ::VR::StereoLayout::TOP_BOTTOM)
                                {
                                    leftFrameData = m_frame.data;
                                    rightFrameData = m_frame.data + halfFrameOffset;
                                }
                                else if (m_preferredStereoLayout == AZ::VR::StereoLayout::BOTTOM_TOP)
                                {
                                    leftFrameData = m_frame.data + halfFrameOffset;
                                    rightFrameData = m_frame.data;
                                }
                            }
                        }
                        
                        if (m_isStereo && gEnv->pRenderer->GetIStereoRenderer()->IsRenderingToHMD())
                        {
                            //If the video playback type could not be determined; don't update the textures
                            if (leftFrameData != nullptr && rightFrameData != nullptr)
                            {
                                gEnv->pRenderer->UpdateTextureInVideoMemory(m_videoTextureLeft, leftFrameData, 0, 0, frameWidth, frameHeight, eTF_R8G8B8A8, 0, 1);
                                gEnv->pRenderer->UpdateTextureInVideoMemory(m_videoTextureRight, rightFrameData, 0, 0, frameWidth, frameHeight, eTF_R8G8B8A8, 0, 1);
                            }
                        }
                        else
                        {
                            //If the video is stereo but we're not in VR, we only need to update the left texture
                            //If the video isn't stereo, the width and height won't be cut so the whole frame will be displayed
                            gEnv->pRenderer->UpdateTextureInVideoMemory(m_videoTextureLeft, leftFrameData, 0, 0, frameWidth, frameHeight, eTF_R8G8B8A8, 0, 1);
                        }

                        //Let the decoder know we're done with the frame
                        m_videoDecoder.Presented();

                        //Note: we don't just set secondsSinceLastFrame to 0 here because we want to keep timing stable.
                        //If we're constantly 0.1 seconds late to display a frame, we want that lateness to accumulate so we eventually
                        //jump ahead 2 frames and stay on time even if we're running late. 
                        m_secondsSinceLastFrame -= (playbackSpeed * framesAhead);
                    }

                    //When the decoder is done processing the movie we want to send out a notification and then seek back to the beginning 
                    //Seeking removes the "finished" flag from the decoder
                    //If we don't want to loop we set playing to false. Otherwise it will just keep playing from the beginning
                    if (m_videoDecoder.IsFinished())
                    {
                        EBUS_EVENT_ID(m_entityId, VideoPlaybackFramework::VideoPlaybackNotificationBus, OnPlaybackFinished);
                        m_videoDecoder.Seek(INT64_MIN);
                        if (!m_shouldLoop)
                        {
                            m_playing = false;
                        }
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Editor events
        AZ::Crc32 VideoPlaybackGameComponent::OnRenderTextureChange()
        {
            //Render texture name must start with a $ to mark that it is a render texture. This system does not work without render textures
            if (m_userTextureName[0] != '$')
            {
                m_userTextureName = "$" + m_userTextureName;
            }

            return AZ::Edit::PropertyRefreshLevels::ValuesOnly;
        }
        //////////////////////////////////////////////////////////////////////////

    } //namespace VideoPlayback
}//namespace AZ

#endif
