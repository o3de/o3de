/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        builderDescriptor.m_version = 5; // bump this to rebuild everything.
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
