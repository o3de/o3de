/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

namespace VideoPlaybackFramework
{
    class VideoPlaybackRequests
        : public AZ::EBusTraits
    {

    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;
        //////////////////////////////////////////////////////////////////////////

        VideoPlaybackRequests() = default;
        virtual ~VideoPlaybackRequests() = default;

        /*
        * Start/resume playing a movie that is attached to the current entity.
        */
        virtual void Play() = 0;

        /*
        * Pause a movie that is attached to the current entity.
        */
        virtual void Pause() = 0;

        /*
        * Stop playing a movie that is attached to the current entity.
        */
        virtual void Stop() = 0;

        /*
        * Returns whether or not the video is currently playing
        * 
        * @return True if the video is currently playing. False if the video is paused or stopped
        */
        virtual bool IsPlaying() = 0;

        /*
        * Get how many frames ahead the decoder should try to be when decoding this video
        *
        * @return Returns How many frames ahead the decoder should try to be when decoding this video
        */
        virtual AZ::u32 GetQueueAheadCount() = 0;

        /* Sets how many frames ahead the decoder should try to be when decoding this video
        *
        * @param queueAheadCount How many frames ahead the decoder should try to be when decoding this video
        */
        virtual void SetQueueAheadCount(AZ::u32 queueAheadCount) = 0;

        /*
        * Get whether or not the movie attached to the current entity should loop
        *
        * @return True if the video is set to loop. False otherwise
        */
        virtual bool GetIsLooping() = 0;

        /*
        * Set whether or not the movie attached to the current entity should loop
        *
        * @param isLooping Whether the video should loop
        */
        virtual void SetIsLooping(bool isLooping) = 0;

        /*
        * Get whether or not the video should start playing automatically on activate
        *
        * @return True if the video is set to start playing automatically. False otherwise
        */
        virtual bool GetIsAutoPlay() = 0;

        /*
        * Set whether or not the video should start playing automatically on activate
        *
        * @param isAutoPlay Whether the video should play automatically
        */
        virtual void SetIsAutoPlay(bool isAutoPlay) = 0;

        /*
        * Get the playback speed factor
        *
        * @return Returns the playback speed factor
        */
        virtual float GetPlaybackSpeed() = 0;

        /* Sets the playback speed based on a factor of the current playback speed
        *
        * For example you can play at half speed by passing 0.5f or play at double speed by
        * passing 2.0f. 
        *
        * @param speedFactor The speed modification factor to apply to playback speed
        */
        virtual void SetPlaybackSpeed(float speedFactor) = 0;

        /*
        * Get the source location of the video
        * @return Returns the source location of the video
        */
        virtual AZStd::string GetVideoPathname() const  = 0;

        /*
        * Set the source location of the video
        *
        * @param videoPath The source location of the video
        */
        virtual void SetVideoPathname(const AZStd::string& videoPath) = 0;

        virtual AZStd::string GetDestinationTextureName() const = 0;
        virtual void SetDestinationTextureName(const AZStd::string& destinationTextureName) = 0;
    };
    using VideoPlaybackRequestBus = AZ::EBus<VideoPlaybackRequests>;


    class VideoPlaybackNotifications
        : public AZ::EBusTraits
    {

    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::EntityId BusIdType;
        //////////////////////////////////////////////////////////////////////////

        VideoPlaybackNotifications() = default;
        virtual ~VideoPlaybackNotifications() = default;

        /*
        * Event that fires when the movie starts playback.
        */
        virtual void OnPlaybackStarted() = 0;

        /*
        * Event that fires when the movie pauses playback.
        */
        virtual void OnPlaybackPaused() = 0;

        /*
        * Event that fires when the movie stops playback.
        */
        virtual void OnPlaybackStopped() = 0;

        /*
        * Event that fires when the movie completes playback.
        */
        virtual void OnPlaybackFinished() = 0;

        /*
        * Event that fires when the first frame gets presented.
        */
        virtual void OnFirstFramePresented() = 0;
    };
    using VideoPlaybackNotificationBus = AZ::EBus<VideoPlaybackNotifications>;
} // namespace VideoPlaybackFramework
