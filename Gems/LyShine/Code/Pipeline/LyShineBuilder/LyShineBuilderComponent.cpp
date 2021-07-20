/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pipeline/LyShineBuilder/LyShineBuilderComponent.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <AzToolsFramework/Fingerprinting/TypeFingerprinter.h>
#include <LyShine/UiComponentTypes.h>
#include <LyShine/UiAssetTypes.h>

namespace LyShine
{
    namespace LyShineBuilder
    {
        void LyShineBuilderComponent::Activate()
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "SerializeContext not found");

            AzToolsFramework::Fingerprinting::TypeFingerprinter fingerprinter(*serializeContext);
            AzToolsFramework::Fingerprinting::TypeCollection types = fingerprinter.GatherAllTypesForComponents();
            AzToolsFramework::Fingerprinting::TypeFingerprint allComponents = fingerprinter.GenerateFingerprintForAllTypes(types);
            AZStd::string builderAnalysisFingerprint = AZStd::string::format("%zu", allComponents);

            // Register UI Canvas Builder
            AssetBuilderSDK::AssetBuilderDesc uiBuilderDescriptor;
            uiBuilderDescriptor.m_name = "UI Canvas Builder";
            uiBuilderDescriptor.m_version = 3;
            uiBuilderDescriptor.m_analysisFingerprint = builderAnalysisFingerprint;
            uiBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.uicanvas", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
            uiBuilderDescriptor.m_busId = UiCanvasBuilderWorker::GetUUID();
            uiBuilderDescriptor.m_createJobFunction = AZStd::bind(&UiCanvasBuilderWorker::CreateJobs, &m_uiCanvasBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
            uiBuilderDescriptor.m_processJobFunction = AZStd::bind(&UiCanvasBuilderWorker::ProcessJob, &m_uiCanvasBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
            m_uiCanvasBuilder.BusConnect(uiBuilderDescriptor.m_busId);

            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, uiBuilderDescriptor);

            AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::RegisterSourceAssetType, azrtti_typeid<LyShine::CanvasAsset>(), LyShine::CanvasAsset::GetFileFilter());
        }

        void LyShineBuilderComponent::Deactivate()
        {
            // Finish all queued work
            AZ::Data::AssetBus::ExecuteQueuedEvents();

            AzToolsFramework::ToolsAssetSystemBus::Broadcast(&AzToolsFramework::ToolsAssetSystemRequests::UnregisterSourceAssetType, azrtti_typeid<LyShine::CanvasAsset>());

            m_uiCanvasBuilder.BusDisconnect();
        }

        void LyShineBuilderComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                // Need to add the AssetBuilder tag because this builder is in a Gem
                // If the tag is not set, the Asset Processor will never use this builder
                serializeContext->Class<LyShineBuilderComponent, AZ::Component>()
                    ->Version(1)
                    ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
            }
        }

    } // namespace LyShineBuilder
} // namespace LyShine
