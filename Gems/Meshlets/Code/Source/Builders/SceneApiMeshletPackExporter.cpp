/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Builders/SceneApiMeshletPackExporter.h>
#include <Builders/MeshletPackRule.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>

namespace AZ::Meshlets::Builders
{

    SceneApiMeshletPackExporter::SceneApiMeshletPackExporter()
    {
        // BindToCall signature varies across O3DE versions; the stubbed export
        // path doesn't need the binding to function (it returns Ignored
        // unconditionally). SP1.5 will restore the binding when the export
        // path is implemented for real.
    }

    void SceneApiMeshletPackExporter::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SceneApiMeshletPackExporter,
                                    AZ::SceneAPI::SceneCore::ExportingComponent>()
                ->Version(1);
        }
    }

    AZ::SceneAPI::Events::ProcessingResult
    SceneApiMeshletPackExporter::ProcessMeshletPackContext(
        AZ::SceneAPI::Events::ICallContext* /*context*/)
    {
        // SP1.5 follow-up: the SceneAPI export path is stubbed. The JSON sidecar
        // builder is the live ingestion path for SP1 (.meshletpack files
        // referencing an .azmodel). The SceneAPI scenemanifest rule UI surface
        // for adding MeshletPackRule to an FBX still works (the rule class is
        // reflected by MeshletsBuildersSystemComponent), but exporting the
        // resulting .azmeshletpack from the SceneAPI graph needs an updated
        // walk against the current SceneAPI API surface -- too much drift from
        // the SP1 plan's reference patterns to land safely in this sweep.
        AZ_TracePrintf("Meshlets.SceneBuilder",
            "SceneAPI export path stubbed for SP1. Use a .meshletpack JSON "
            "sidecar referencing the model's AssetId instead.\n");
        return AZ::SceneAPI::Events::ProcessingResult::Ignored;
    }

} // namespace AZ::Meshlets::Builders
