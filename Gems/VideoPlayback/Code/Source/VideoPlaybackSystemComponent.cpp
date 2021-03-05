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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzCore/Asset/AssetManagerBus.h>

#include "VideoPlaybackSystemComponent.h"

namespace AZ
{
    namespace VideoPlayback
    {
        void VideoPlaybackSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<VideoPlaybackSystemComponent, AZ::Component>()
                    ->Version(0);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    auto editInfo = editContext->Class<VideoPlaybackSystemComponent>("Video Playback", "These are the properties of the Video Playback.");
                    editInfo->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Rendering")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        void VideoPlaybackSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("VideoPlaybackService"));
        }

        void VideoPlaybackSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("VideoPlaybackService"));
        }

        void VideoPlaybackSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("AssetDatabaseService"));
        }

        void VideoPlaybackSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("AssetCatalogService"));
        }

        void VideoPlaybackSystemComponent::Init()
        {
        }

        void VideoPlaybackSystemComponent::Activate()
        {
        }

        void VideoPlaybackSystemComponent::Deactivate()
        {
        }
    }//namespace VideoPlayback
}//namespace AZ
