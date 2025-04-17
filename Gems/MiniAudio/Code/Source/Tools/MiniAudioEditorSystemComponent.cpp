/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MiniAudioEditorSystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <MiniAudio/SoundAsset.h>

namespace MiniAudio
{
    AZ::ComponentDescriptor* MiniAudioEditorSystemComponent_CreateDescriptor()
    {
        return MiniAudioEditorSystemComponent::CreateDescriptor();
    }

    AZ::TypeId MiniAudioEditorSystemComponent_GetTypeId()
    {
        return azrtti_typeid<MiniAudioEditorSystemComponent>();
    }

    void MiniAudioEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MiniAudioEditorSystemComponent, AZ::Component>()->Version(1)->Attribute(
                AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }));
        }
    }

    void MiniAudioEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("MiniAudioEditorService"));
    }

    void MiniAudioEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("MiniAudioEditorService"));
    }

    void MiniAudioEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void MiniAudioEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void MiniAudioEditorSystemComponent::Activate()
    {
        MiniAudioSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();

        // Register MiniSound Asset
        auto* materialAsset =
            aznew AzFramework::GenericAssetHandler<SoundAsset>("MiniSound Asset", SoundAsset::AssetGroup, SoundAsset::FileExtension);
        materialAsset->Register();
        m_assetHandlers.emplace_back(materialAsset);

        // Register MiniSound Asset Builder
        AssetBuilderSDK::AssetBuilderDesc materialAssetBuilderDescriptor;
        materialAssetBuilderDescriptor.m_name = "MiniSound Asset Builder";
        materialAssetBuilderDescriptor.m_version = 3; // bump this to rebuild all sound files
        materialAssetBuilderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.ogg", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        materialAssetBuilderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.flac", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        materialAssetBuilderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.mp3", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        materialAssetBuilderDescriptor.m_patterns.push_back(
            AssetBuilderSDK::AssetBuilderPattern("*.wav", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        materialAssetBuilderDescriptor.m_busId = azrtti_typeid<SoundAssetBuilder>();
        materialAssetBuilderDescriptor.m_createJobFunction =
            [this](const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
        {
            m_soundAssetBuilder.CreateJobs(request, response);
        };
        materialAssetBuilderDescriptor.m_processJobFunction =
            [this](const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
        {
            m_soundAssetBuilder.ProcessJob(request, response);
        };
        m_soundAssetBuilder.BusConnect(materialAssetBuilderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(
            &AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, materialAssetBuilderDescriptor);
    }

    void MiniAudioEditorSystemComponent::Deactivate()
    {
        m_assetHandlers.clear();

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        MiniAudioSystemComponent::Deactivate();
    }

} // namespace MiniAudio
