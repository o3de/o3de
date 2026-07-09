/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CanvasUpgradeBuilderComponent.h"
#include "CanvasUpgrader.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace LYShineToShine
{
    static const char* const s_builderName = "LYShineToShine Canvas Upgrader";

    void CanvasUpgradeBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CanvasUpgradeBuilderComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags,
                    AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }

    void CanvasUpgradeBuilderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LYShineToShineCanvasUpgradeBuilderService"));
    }

    void CanvasUpgradeBuilderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LYShineToShineCanvasUpgradeBuilderService"));
    }

    void CanvasUpgradeBuilderComponent::Activate()
    {
        AssetBuilderSDK::AssetBuilderDesc builderDesc;
        builderDesc.m_name = s_builderName;
        builderDesc.m_version = 1;
        builderDesc.m_patterns.emplace_back("*.uicanvas", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard);
        builderDesc.m_busId = AZ::Uuid::CreateString("{E5F1A2B3-C4D5-6E7F-8A9B-0C1D2E3F4A5B}");
        builderDesc.m_createJobFunction =
            AZStd::bind(&CanvasUpgradeBuilderComponent::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDesc.m_processJobFunction =
            AZStd::bind(&CanvasUpgradeBuilderComponent::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(builderDesc.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(
            &AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, builderDesc);
    }

    void CanvasUpgradeBuilderComponent::Deactivate()
    {
        BusDisconnect();
    }

    void CanvasUpgradeBuilderComponent::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void CanvasUpgradeBuilderComponent::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request,
        AssetBuilderSDK::CreateJobsResponse& response)
    {
        // Builder disabled — use console commands instead:
        //   1. convert_slices <dir>   (creates .uiprefab files from .slice files)
        //   2. upgrade_canvases <dir> (upgrades v2 canvases to v4 with prefab references)
        // The AP builder cannot guarantee correct ordering (slices must be converted
        // before canvases so that UiPrefabInstance references can be created).
        AZ_UNUSED(request);
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void CanvasUpgradeBuilderComponent::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZ_TracePrintf(s_builderName, "Upgrading canvas: %s\n", request.m_sourceFile.c_str());

        CanvasUpgrader upgrader;
        AZStd::string error;
        AZ::IO::Path sourcePath(request.m_fullPath);

        if (!upgrader.UpgradeFile(sourcePath, error))
        {
            if (error == "already_v3")
            {
                // Race: file was already upgraded (maybe by a concurrent run)
                AZ_TracePrintf(s_builderName, "Canvas already v3, skipping: %s\n", request.m_sourceFile.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
                return;
            }

            AZ_Error(s_builderName, false, "Failed to upgrade canvas '%s': %s",
                request.m_sourceFile.c_str(), error.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AZ_TracePrintf(s_builderName, "Successfully upgraded canvas: %s\n", request.m_sourceFile.c_str());

        // No product output — the converted source will be picked up by AP and
        // processed by the Shine builder to produce the runtime product.
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }
} // namespace LYShineToShine
