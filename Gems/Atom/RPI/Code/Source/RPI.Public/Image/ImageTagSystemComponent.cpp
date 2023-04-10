/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
* @file ImageTagSystemComponent.cpp
* @brief Contains the definition of the ImageTagSystemComponent methods that aren't defined as inline
*/

#include <Atom/RPI.Public/Image/ImageTagSystemComponent.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    namespace RPI
    {
        ImageTagSystemComponent::~ImageTagSystemComponent() = default;

        void ImageTagSystemComponent::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ImageTagSystemComponent::TagData>()
                    ->Version(0)
                    ->Field("Quality", &ImageTagSystemComponent::TagData::m_quality)
                    ->Field("RegisteredImages", &ImageTagSystemComponent::TagData::m_registeredImages)
                ;

                serializeContext->Class<ImageTagSystemComponent, Component>()
                    ->Version(0)
                    ->Field("ImageTags", &ImageTagSystemComponent::m_imageTags)
                ;
            }
        }

        void ImageTagSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }

        void ImageTagSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ImageTagSystemComponent"));
        }

        void ImageTagSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
        {
        }

        void ImageTagSystemComponent::Activate()
        {
            ImageTagBus::Handler::BusConnect();
        }

        void ImageTagSystemComponent::Deactivate()
        {
            ImageTagBus::Handler::BusDisconnect();
        }

        AssetQuality ImageTagSystemComponent::GetQuality(const AZ::Name& imageTag) const
        {
            auto it = m_imageTags.find(imageTag);
            if (it == m_imageTags.end())
            {
                AZ_Warning("ImageTagSystemComponent", false, "Image tag %s has not been registered", imageTag.GetCStr());
                return AssetQualityHighest;
            }

            return it->second.m_quality;
        }

        AZStd::vector<AZ::Name> ImageTagSystemComponent::GetTags() const
        {
            AZStd::vector<AZ::Name> tags;
            for (auto&& [tag, tagData] : m_imageTags)
            {
                tags.push_back(tag);
            }

            AZStd::sort(tags.begin(), tags.end(), [](const AZ::Name& lhs, const AZ::Name& rhs) { return lhs.GetStringView() < rhs.GetStringView(); });

            return tags;
        }

        void ImageTagSystemComponent::RegisterAsset(AZ::Name imageTag, const Data::AssetId& assetId)
        {
            auto it = m_imageTags.find(imageTag);
            if (it == m_imageTags.end())
            {
                AZ_Warning("ImageTagSystemComponent", false, "Image tag %s has not been registered", imageTag.GetCStr());
                return;
            }

            TagData& tagData = it->second;
            tagData.m_registeredImages.insert(assetId);
        }

        void ImageTagSystemComponent::RegisterTag(AZ::Name imageTag)
        {
            AZ_Warning("ImageTagSystemComponent", m_imageTags.find(imageTag) == m_imageTags.end(), "Image tag %s has already been registered", imageTag.GetCStr());

            m_imageTags.emplace(AZStd::move(imageTag), TagData{});
        }

        void ImageTagSystemComponent::SetQuality(const AZ::Name& imageTag, AssetQuality quality)
        {
            auto it = m_imageTags.find(imageTag);
            if (it == m_imageTags.end())
            {
                AZ_Warning("ImageTagSystemComponent", false, "Image tag %s has not been registered", imageTag.GetCStr());
                return;
            }

            TagData& tagData = it->second;
            if (tagData.m_quality == quality)
            {
                return;
            }

            tagData.m_quality = quality;
            ImageTagNotificationBus::Event(imageTag, &AssetTagNotification<ImageAsset>::OnAssetTagQualityUpdated, quality);

            for (const AZ::Data::AssetId& assetId : tagData.m_registeredImages)
            {
                AZ::SystemTickBus::QueueFunction([assetId]
                {
                    AZ::Data::AssetManager::Instance().ReloadAsset(assetId, AZ::Data::AssetLoadBehavior::Default);
                });
            }
        }
    } // namespace RPI
} // namespace AZ
