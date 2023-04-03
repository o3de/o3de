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
    //! This is the component responsible for actually hooking into the scene API's processing flow
    //! during the generation step.
    class UVsPreExportComponent
        : public AZ::SceneAPI::SceneCore::GenerationComponent
    {
    public:
        AZ_COMPONENT(UVsPreExportComponent, "{64F79C1E-CED6-42A9-8229-6607F788C731}", AZ::SceneAPI::SceneCore::GenerationComponent)

        UVsPreExportComponent();

        static void Reflect(AZ::ReflectContext* context);

        AZ::SceneAPI::Events::ProcessingResult Register(AZ::SceneAPI::Events::GenerateAdditionEventContext& context);
    };
} // namespace AZ::SceneGenerationComponents
