/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AzCore/std/containers/map.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Builder/ScriptCanvasBuilderComponent.h>
#include <Builder/ScriptCanvasBuilderWorker.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAsset.h>
#include <ScriptCanvas/Asset/SubgraphInterfaceAssetHandler.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>

namespace ScriptCanvasBuilder
{
    template<typename t_Asset, typename t_Handler>
    HandlerOwnership RegisterHandler(const char* extension, bool enableCatalog)
    {
        AZ::Data::AssetType assetType(azrtti_typeid<t_Asset>());
        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);

        if (enableCatalog)
        {
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
        }

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension, extension);

        if (auto borrowedHandler = AZ::Data::AssetManager::Instance().GetHandler(assetType))
        {
            return { borrowedHandler, false };
        }
        else
        {
            t_Handler* ownedHandler = aznew t_Handler();
            AZ::Data::AssetManager::Instance().RegisterHandler(ownedHandler, assetType);
            return { ownedHandler, true };
        }
    }

    SharedHandlers HandleAssetTypes()
    {
        SharedHandlers handlers;
        handlers.m_subgraphInterfaceHandler = RegisterHandler<ScriptCanvas::SubgraphInterfaceAsset, ScriptCanvas::SubgraphInterfaceAssetHandler>("scriptcanvas_fn_compiled", true);
        handlers.m_runtimeAssetHandler = RegisterHandler<ScriptCanvas::RuntimeAsset, JobDependencyVerificationHandler>("scriptcanvas_compiled", true);

        // \todo make it so we can load script events in the builder: expose the SE handler?
        // const AZStd::string description = ScriptCanvas::AssetDescription::GetExtension<ScriptCanvas::RuntimeAsset>();
        // handlers.m_scriptEventAssetHandler = RegisterHandler<ScriptEvents::ScriptEventsAsset, ScriptEventsEditor::ScriptEventAssetHandler>(description.data());

        return handlers;
    }

    void PluginComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptCanvasBuilderService"));
    }
    
    void PluginComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("ScriptCanvasService"));
        required.push_back(AZ_CRC_CE("ScriptCanvasReflectService"));
    }
    
    void PluginComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
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
            size_t fingerprint = ScriptCanvas::BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
            builderDescriptor.m_analysisFingerprint = AZStd::string(m_scriptCanvasBuilder.GetFingerprintString())
                .append("|").append(AZStd::to_string(static_cast<AZ::u64>(fingerprint)));

            // Include the base node version in the hash, so when it changes, script canvas jobs are reprocessed.
            AZStd::hash_combine(fingerprint, ScriptCanvas::Node::GetNodeVersion());

            builderDescriptor.AddFlags(AssetBuilderSDK::AssetBuilderDesc::BF_DeleteLastKnownGoodProductOnFailure, s_scriptCanvasProcessJobKey);
            builderDescriptor.m_productsToKeepOnFailure[s_scriptCanvasProcessJobKey] = { AZ_CRC_CE("SubgraphInterface") };
            m_scriptCanvasBuilder.BusConnect(builderDescriptor.m_busId);
            AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBus::Handler::RegisterBuilderInformation, builderDescriptor);
        }

        m_sharedHandlers = HandleAssetTypes();
        AssetHandlers workerHandlers(m_sharedHandlers);
        m_scriptCanvasBuilder.Activate(workerHandlers);
    }

    void PluginComponent::Deactivate()
    {
        // Finish all queued work
        AZ::Data::AssetBus::ExecuteQueuedEvents();
        m_scriptCanvasBuilder.BusDisconnect();
        m_sharedHandlers.DeleteOwnedHandlers();
    }

    void PluginComponent::Reflect(AZ::ReflectContext* context)
    {
        BuildVariableOverrides::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PluginComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
                ;
        }
    }
}
