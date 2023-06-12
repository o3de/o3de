/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
* @file ModelTagSystemComponent.cpp
* @brief Contains the definition of the ModelTagSystemComponent methods that aren't defined as inline
*/

#include <Atom/RPI.Public/Model/ModelTagSystemComponent.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/sort.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    namespace RPI
    {
        ModelTagSystemComponent::~ModelTagSystemComponent() = default;

        void ModelTagSystemComponent::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ModelTagSystemComponent::TagData>()
                    ->Version(0)
                    ->Field("Quality", &ModelTagSystemComponent::TagData::quality)
                    ->Field("RegisteredModels", &ModelTagSystemComponent::TagData::registeredModels)
                ;

                serializeContext->Class<ModelTagSystemComponent, Component>()
                    ->Version(0)
                    ->Field("ModelTags", &ModelTagSystemComponent::m_modelTags)
                ;
            }
        }

        void ModelTagSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }

        void ModelTagSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ModelTagSystemComponent"));
        }

        void ModelTagSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& /*dependent*/)
        {
        }

        void ModelTagSystemComponent::Activate()
        {
            ModelTagBus::Handler::BusConnect();
        }

        void ModelTagSystemComponent::Deactivate()
        {
            ModelTagBus::Handler::BusDisconnect();
        }

        AssetQuality ModelTagSystemComponent::GetQuality(const AZ::Name& modelTag) const
        {
            auto it = m_modelTags.find(modelTag);
            if (it == m_modelTags.end())
            {
                AZ_Warning("ModelTagSystemComponent", false, "Model tag %s has not been registered", modelTag.GetCStr());
                return AssetQualityHighest;
            }

            return it->second.quality;
        }

        AZStd::vector<AZ::Name> ModelTagSystemComponent::GetTags() const
        {
            AZStd::vector<AZ::Name> tags;
            for (auto&& [tag, tagData] : m_modelTags)
            {
                tags.push_back(tag);
            }

            AZStd::sort(tags.begin(), tags.end(), [](const AZ::Name& lhs, const AZ::Name& rhs) { return lhs.GetStringView() < rhs.GetStringView(); });

            return tags;
        }

        void ModelTagSystemComponent::RegisterAsset(AZ::Name modelTag, const Data::AssetId& assetId)
        {
            auto it = m_modelTags.find(modelTag);
            if (it == m_modelTags.end())
            {
                AZ_Warning("ModelTagSystemComponent", false, "Model tag %s has not been registered", modelTag.GetCStr());
                return;
            }

            TagData& tagData = it->second;
            tagData.registeredModels.insert(assetId);
        }

        void ModelTagSystemComponent::RegisterTag(AZ::Name modelTag)
        {
            AZ_Warning("ModelTagSystemComponent", m_modelTags.find(modelTag) == m_modelTags.end(), "Model tag %s has already been registered", modelTag.GetCStr());

            m_modelTags.emplace(AZStd::move(modelTag), TagData{});
        }

        void ModelTagSystemComponent::SetQuality(const AZ::Name& modelTag, AssetQuality quality)
        {
            auto it = m_modelTags.find(modelTag);
            if (it == m_modelTags.end())
            {
                AZ_Warning("ModelTagSystemComponent", false, "Model tag %s has not been registered", modelTag.GetCStr());
                return;
            }

            TagData& tagData = it->second;
            if (tagData.quality == quality)
            {
                return;
            }

            tagData.quality = quality;
            ModelTagNotificationBus::Event(modelTag, &AssetTagNotification<ModelAsset>::OnAssetTagQualityUpdated, quality);

            for (const AZ::Data::AssetId& assetId : tagData.registeredModels)
            {
                AZ::SystemTickBus::QueueFunction([assetId]
                {
                    AzFramework::AssetCatalogEventBus::Broadcast(&AzFramework::AssetCatalogEventBus::Events::OnCatalogAssetChanged, assetId);
                });
            }
        }
    } // namespace RPI
} // namespace AZ
