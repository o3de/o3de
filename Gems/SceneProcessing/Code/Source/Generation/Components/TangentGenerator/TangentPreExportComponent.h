/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

#include <AzCore/RTTI/RTTI.h>


namespace AZ::SceneAPI::Events { class GenerateAdditionEventContext; }

namespace AZ::SceneGenerationComponents
{
    class TangentPreExportComponent
        : public AZ::SceneAPI::SceneCore::GenerationComponent
    {
    public:
        AZ_COMPONENT(TangentPreExportComponent, "{BFFE114A-2FC6-42F1-92C4-61329CC54A2B}", AZ::SceneAPI::SceneCore::GenerationComponent)

        TangentPreExportComponent();

        static void Reflect(AZ::ReflectContext* context);

        // bumps Tangent export to later on in the generation phase, so that it can generate tangents after other rules have
        // generated things like normals and UVs.

        uint8_t GetPriority() const override;

        AZ::SceneAPI::Events::ProcessingResult Register(AZ::SceneAPI::Events::GenerateAdditionEventContext& context);
    };
} // namespace AZ::SceneGenerationComponents
