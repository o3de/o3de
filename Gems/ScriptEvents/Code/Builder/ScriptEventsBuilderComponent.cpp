/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builder/ScriptEventsBuilderComponent.h>

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>

namespace ScriptEventsBuilder
{
    void ScriptEventsBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptEventsBuilderService"));
    }
    
    void ScriptEventsBuilderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }
    
    void ScriptEventsBuilderComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
    }
    
    void ScriptEventsBuilderComponent::Activate()
    {
        // Register ScriptEvents Builder
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "Script Events Builder";
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.scriptevents", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = ScriptEventsBuilder::Worker::GetUUID();
        builderDescriptor.m_createJobFunction = AZStd::bind(&Worker::CreateJobs, &m_scriptEventsBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&Worker::ProcessJob, &m_scriptEventsBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        // changing the version number invalidates all assets and will rebuild everything.
        builderDescriptor.m_version = m_scriptEventsBuilder.GetVersionNumber();
        // changing the analysis fingerprint just invalidates analysis (ie, not the assets themselves)
        // which will cause the "CreateJobs" function to be called, for each asset, even if the
        // source file has not changed, but won't actually do the jobs unless the source file has changed
        // or the fingerprint of the individual job is different.
        builderDescriptor.m_analysisFingerprint = m_scriptEventsBuilder.GetFingerprintString();

        m_scriptEventsBuilder.BusConnect(builderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builderDescriptor);

        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), ScriptEvents::ScriptEventsAsset::GetFileFilter());
        m_scriptEventsBuilder.Activate();

    }

    void ScriptEventsBuilderComponent::Deactivate()
    {
        // Finish all queued work
        AZ::Data::AssetBus::ExecuteQueuedEvents();
        
        AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::UnregisterSourceAssetType, azrtti_typeid<ScriptEvents::ScriptEventsAsset>());

        m_scriptEventsBuilder.Deactivate();
        m_scriptEventsBuilder.BusDisconnect();
    }

    void ScriptEventsBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptEventsBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
                ;
        }
    }
}
