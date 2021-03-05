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

#include <precompiled.h>
#include <Builder/ScriptCanvasBuilderComponent.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <Asset/Functions/ScriptCanvasFunctionAsset.h>

namespace ScriptCanvasBuilder
{
    void PluginComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasBuilderService", 0x4929ffcd));
    }
    
    void PluginComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        required.push_back(AZ_CRC("ScriptCanvasReflectService", 0xb3bfe139));
    }
    
    void PluginComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }
    
    void PluginComponent::Activate()
    {
        // Register ScriptCanvas Builder
        {
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
            builderDescriptor.m_name = "Script Canvas Builder";
            builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.scriptcanvas", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            builderDescriptor.m_busId = ScriptCanvasBuilder::Worker::GetUUID();
            builderDescriptor.m_createJobFunction = AZStd::bind(&Worker::CreateJobs, &m_scriptCanvasBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction = AZStd::bind(&Worker::ProcessJob, &m_scriptCanvasBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
            // changing the version number invalidates all assets and will rebuild everything.
            builderDescriptor.m_version = m_scriptCanvasBuilder.GetVersionNumber();
            // changing the analysis fingerprint just invalidates analysis (ie, not the assets themselves)
            // which will cause the "CreateJobs" function to be called, for each asset, even if the
            // source file has not changed, but won't actually do the jobs unless the source file has changed
            // or the fingerprint of the individual job is different.
            builderDescriptor.m_analysisFingerprint = m_scriptCanvasBuilder.GetFingerprintString();

            m_scriptCanvasBuilder.BusConnect(builderDescriptor.m_busId);
            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builderDescriptor);

            AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>(), ScriptCanvasEditor::ScriptCanvasAsset::Description::GetFileFilter<ScriptCanvasEditor::ScriptCanvasAsset>());
            m_scriptCanvasBuilder.Activate();
        }

        {
            AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
            builderDescriptor.m_name = "Script Canvas Function Builder";
            builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.scriptcanvas_fn", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            builderDescriptor.m_busId = ScriptCanvasBuilder::FunctionWorker::GetUUID();
            builderDescriptor.m_createJobFunction = AZStd::bind(&FunctionWorker::CreateJobs, &m_scriptCanvasFunctionBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
            builderDescriptor.m_processJobFunction = AZStd::bind(&FunctionWorker::ProcessJob, &m_scriptCanvasFunctionBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
            // changing the version number invalidates all assets and will rebuild everything.
            builderDescriptor.m_version = m_scriptCanvasFunctionBuilder.GetVersionNumber();
            // changing the analysis fingerprint just invalidates analysis (ie, not the assets themselves)
            // which will cause the "CreateJobs" function to be called, for each asset, even if the
            // source file has not changed, but won't actually do the jobs unless the source file has changed
            // or the fingerprint of the individual job is different.
            builderDescriptor.m_analysisFingerprint = m_scriptCanvasFunctionBuilder.GetFingerprintString();

            m_scriptCanvasFunctionBuilder.BusConnect(builderDescriptor.m_busId);
            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builderDescriptor);

            AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>(), ScriptCanvas::ScriptCanvasFunctionAsset::Description::GetFileFilter<ScriptCanvas::ScriptCanvasFunctionAsset>());
            m_scriptCanvasFunctionBuilder.Activate();
        }
    }

    void PluginComponent::Deactivate()
    {
        // Finish all queued work
        AZ::Data::AssetBus::ExecuteQueuedEvents();
        
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::UnregisterSourceAssetType, azrtti_typeid<ScriptCanvasEditor::ScriptCanvasAsset>());
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::UnregisterSourceAssetType, azrtti_typeid<ScriptCanvas::ScriptCanvasFunctionAsset>());

        m_scriptCanvasBuilder.Deactivate();
        m_scriptCanvasBuilder.BusDisconnect();
    }

    void PluginComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PluginComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
                ;
        }
    }
}
