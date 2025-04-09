/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include "VideoPlaybackFrameworkSystemComponent.h"
#include "Include/VideoPlaybackFramework/VideoPlaybackAsset.h"
#include "Include/VideoPlaybackFramework/VideoPlaybackBus.h"

namespace VideoPlaybackFramework
{
    class BehaviorVideoPlaybackNotificationBusHandler
        : public VideoPlaybackNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorVideoPlaybackNotificationBusHandler, "{F3116FA1-3F81-4ADE-9941-C5A5C838197B}", AZ::SystemAllocator,
            OnPlaybackStarted,
            OnPlaybackPaused,
            OnPlaybackStopped,
            OnPlaybackFinished,
            OnFirstFramePresented
            );

        // Sent when playback starts or resumes
        void OnPlaybackStarted() override
        {
            Call(FN_OnPlaybackStarted);
        }

        // Sent when the video is paused
        void OnPlaybackPaused() override
        {
            Call(FN_OnPlaybackPaused);
        }

        // Sent when the video is stopped
        void OnPlaybackStopped() override
        {
            Call(FN_OnPlaybackStopped);
        }

        // Sent when the video finishes
        void OnPlaybackFinished() override
        {
            Call(FN_OnPlaybackFinished);
        }

        // Sent when the video decodes first frame
        void OnFirstFramePresented() override
        {
            Call(FN_OnFirstFramePresented);
        }
    };

    void VideoPlaybackFrameworkSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
           AzFramework::SimpleAssetReference<VideoPlaybackAsset>::Register(*serialize);

            serialize->Class<VideoPlaybackFrameworkSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<VideoPlaybackFrameworkSystemComponent>("VideoPlaybackFramework", "Interface framework to play back video during gameplay.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<VideoPlaybackRequestBus>("VideoPlaybackRequestBus")
                ->Event("Play", &VideoPlaybackRequestBus::Events::Play)
                ->Event("Pause", &VideoPlaybackRequestBus::Events::Pause)
                ->Event("Stop", &VideoPlaybackRequestBus::Events::Stop)
                ->Event("IsPlaying", &VideoPlaybackRequestBus::Events::IsPlaying)
                ->Event("GetQueueAheadCount", &VideoPlaybackRequestBus::Events::GetQueueAheadCount)
                ->Event("SetQueueAheadCount", &VideoPlaybackRequestBus::Events::SetQueueAheadCount)
                ->Event("GetIsLooping", &VideoPlaybackRequestBus::Events::GetIsLooping)
                ->Event("SetIsLooping", &VideoPlaybackRequestBus::Events::SetIsLooping)
                ->Event("GetIsAutoPlay", &VideoPlaybackRequestBus::Events::GetIsAutoPlay)
                ->Event("SetIsAutoPlay", &VideoPlaybackRequestBus::Events::SetIsAutoPlay)
                ->Event("GetPlaybackSpeed", &VideoPlaybackRequestBus::Events::GetPlaybackSpeed)
                ->Event("SetPlaybackSpeed", &VideoPlaybackRequestBus::Events::SetPlaybackSpeed)
                ->Event("GetVideoPathname", &VideoPlaybackRequestBus::Events::GetVideoPathname)
                ->Event("SetVideoPathname", &VideoPlaybackRequestBus::Events::SetVideoPathname)
                ->Event("GetDestinationTextureName", &VideoPlaybackRequestBus::Events::GetDestinationTextureName)
                ->Event("SetDestinationTextureName", &VideoPlaybackRequestBus::Events::SetDestinationTextureName)
                ;

            behaviorContext->EBus<VideoPlaybackNotificationBus>("VideoPlaybackNotificationBus")
                ->Handler<BehaviorVideoPlaybackNotificationBusHandler>()
                ;
        }
    }

    void VideoPlaybackFrameworkSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("VideoPlaybackFrameworkService"));
    }

    void VideoPlaybackFrameworkSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("VideoPlaybackFrameworkService"));
    }

    void VideoPlaybackFrameworkSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void VideoPlaybackFrameworkSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void VideoPlaybackFrameworkSystemComponent::Init()
    {
    }

    void VideoPlaybackFrameworkSystemComponent::Activate()
    {
        VideoPlaybackFrameworkRequestBus::Handler::BusConnect();

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            [](AZ::Data::AssetCatalogRequests* handler)
            {
                handler->EnableCatalogForAsset(azrtti_typeid<VideoPlaybackAsset>());
                handler->AddExtension("mp4");
                handler->AddExtension("mkv");
                handler->AddExtension("webm");
                handler->AddExtension("mov");
            });
    }

    void VideoPlaybackFrameworkSystemComponent::Deactivate()
    {
        VideoPlaybackFrameworkRequestBus::Handler::BusDisconnect();
    }
}
