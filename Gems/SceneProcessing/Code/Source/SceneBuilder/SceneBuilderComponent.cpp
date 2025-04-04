/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <SceneBuilder/SceneBuilderComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace SceneBuilder
{
    void BuilderPluginComponent::Activate()
    {
        using namespace AZ::SceneAPI::Events;

        AZStd::unordered_set<AZStd::string> extensions;
        AssetImportRequestBus::Broadcast(&AssetImportRequestBus::Events::GetSupportedFileExtensions, extensions);

        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Scene Builder";
        for (const AZStd::string& extension : extensions)
        {
            if (extension.empty())
            {
                continue;
            }
            AZStd::string pattern = AZStd::string::format((extension[0] == '.' ? "*%s" : "*.%s"), extension.c_str());
            builderDescriptor.m_patterns.emplace_back(AZStd::move(pattern), AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        }
        builderDescriptor.m_busId = SceneBuilderWorker::GetUUID();
        builderDescriptor.m_createJobFunction = AZStd::bind(&SceneBuilderWorker::CreateJobs, &m_sceneBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&SceneBuilderWorker::ProcessJob, &m_sceneBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);

        builderDescriptor.m_version = 13; // Add BufferAssetAllocator
        builderDescriptor.m_analysisFingerprint = m_sceneBuilder.GetFingerprint(); // bump this to at least re-analyze everything.

        m_sceneBuilder.BusConnect(builderDescriptor.m_busId);

        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Events::RegisterBuilderInformation, builderDescriptor);

        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Creating entity with scene system components.\n");

        AZ::Entity* sceneSystemEntity = AZ::SceneAPI::SceneCore::EntityConstructor::BuildSceneSystemEntity();

        AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, sceneSystemEntity, "Unable to create a system component for the SceneAPI.\n");
        if (sceneSystemEntity)
        {
            sceneSystemEntity->Init();
            sceneSystemEntity->Activate();
        }
    }

    void BuilderPluginComponent::Deactivate()
    {
        m_sceneBuilder.BusDisconnect();
    }

    void BuilderPluginComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.emplace_back(AZ_CRC_CE("AssetImportRequestHandler"));
    }

    void BuilderPluginComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        // Any components that can modify the analysis fingerprint via SceneBuilderDependencyRequests::AddFingerprintInfo must be activated first,
        // so they contribute to the fingerprint calculated in BuilderPluginComponent::Activate().
        services.emplace_back(AZ_CRC_CE("FingerprintModification"));
    }

    void BuilderPluginComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<BuilderPluginComponent, AZ::Component>()->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }
} // namespace SceneBuilder
