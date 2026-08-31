/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Builders/MeshletsBuildersSystemComponent.h>
#include <Builders/JsonSidecarDescriptor.h>
#include <Builders/MeshletPackRule.h>
#include <Builders/MeshletPackRuleBehavior.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>

namespace AZ::Meshlets::Builders
{
    void MeshletsBuildersSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // The AssetBuilder system component tag is what triggers
            // AssetBuilder.exe to auto-instantiate + activate this component
            // when it loads Meshlets.Builders.dll. Without it the DLL loads
            // fine but Activate() never runs, so JsonSidecarBuilder never
            // registers with AssetBuilderBus and AP never processes .meshletpack
            // sidecar files. Matches the pattern used by every other Atom
            // builder system component (e.g. AzslShaderBuilderSystemComponent,
            // ImageProcessingAtom::BuilderPluginComponent).
            serializeContext->Class<MeshletsBuildersSystemComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags,
                    AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
        MeshletPackRule::Reflect(context);
        // NOTE: MeshletPackRuleBehavior is a Component registered as a descriptor in
        // MeshletsBuildersModule (CreateDescriptor), which reflects it automatically.
        // Reflecting it here TOO caused a duplicate-Uuid assert in AssetBuilder
        // ("MeshletPackRuleBehavior could not be registered with duplicated Uuid"),
        // exactly like the descriptor-only SceneApiMeshletPackExporter (not reflected
        // here either). So we do NOT manually reflect it.
        JsonSidecarDescriptor::Reflect(context);
    }

    void MeshletsBuildersSystemComponent::GetProvidedServices(
        AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MeshletsBuildersService"));
    }

    void MeshletsBuildersSystemComponent::GetIncompatibleServices(
        AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MeshletsBuildersService"));
    }

    void MeshletsBuildersSystemComponent::Activate()
    {
        // Register the JSON sidecar builder.
        AssetBuilderSDK::AssetBuilderDesc desc;
        desc.m_name = "Meshlet Pack JSON Builder";
        desc.m_busId = AZ::Uuid("{6E7F2D1A-3B5C-4E89-A2D7-9F4C1B5E8A30}");
        desc.m_version = JsonSidecarBuilder::BuilderVersion;
        desc.m_patterns.emplace_back(
            JsonSidecarBuilder::SourceExt,
            AssetBuilderSDK::AssetBuilderPattern::Wildcard);
        desc.m_createJobFunction =
            [this](const AssetBuilderSDK::CreateJobsRequest& req,
                   AssetBuilderSDK::CreateJobsResponse& resp)
            {
                m_jsonBuilder.CreateJobs(req, resp);
            };
        desc.m_processJobFunction =
            [this](const AssetBuilderSDK::ProcessJobRequest& req,
                   AssetBuilderSDK::ProcessJobResponse& resp)
            {
                m_jsonBuilder.ProcessJob(req, resp);
            };
        AssetBuilderSDK::AssetBuilderBus::Broadcast(
            &AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, desc);

        // SceneAPI exporter is registered by SceneAPI's ExportingComponent
        // base class automatically when the module loads — no extra
        // registration call needed here per spike findings.
    }

    void MeshletsBuildersSystemComponent::Deactivate()
    {
        // Deactivate implementation.
    }

} // namespace AZ::Meshlets::Builders
