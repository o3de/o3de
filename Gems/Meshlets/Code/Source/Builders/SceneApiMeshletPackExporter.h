/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>

namespace AZ::Meshlets::Builders
{
    //! SceneAPI export component: when an FBX's IMeshGroup has a
    //! MeshletPackRule, this component produces the sibling .azmeshletpack
    //! product, registers it with AssetBuilderSDK, and declares a product
    //! dependency on the IMeshGroup's .azmodel sibling.
    class SceneApiMeshletPackExporter
        : public AZ::SceneAPI::SceneCore::ExportingComponent
    {
    public:
        AZ_COMPONENT(SceneApiMeshletPackExporter,
                     "{4F8E2C3A-9D7B-4E1F-A6B5-3C8D2E5F7A91}",
                     AZ::SceneAPI::SceneCore::ExportingComponent);

        SceneApiMeshletPackExporter();
        ~SceneApiMeshletPackExporter() override = default;

        static void Reflect(AZ::ReflectContext* context);

        AZ::SceneAPI::Events::ProcessingResult ProcessMeshletPackContext(
            AZ::SceneAPI::Events::ICallContext* context);
    };

} // namespace AZ::Meshlets::Builders
