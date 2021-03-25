/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

#include <RC/ResourceCompilerScene/Common/ExportContextGlobal.h>
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

        AZ::SceneAPI::Events::ProcessingResult Register(AZ::SceneAPI::Events::GenerateAdditionEventContext& context);
    };
} // namespace AZ::SceneGenerationComponents
