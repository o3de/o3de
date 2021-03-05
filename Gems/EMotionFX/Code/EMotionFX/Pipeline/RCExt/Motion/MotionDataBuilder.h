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

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        struct MotionDataBuilderContext;

        class MotionDataBuilder
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(MotionDataBuilder, "{F60AFB28-BB51-463F-BD9F-04C05656EF78}", AZ::SceneAPI::SceneCore::ExportingComponent);

            MotionDataBuilder();
            ~MotionDataBuilder() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult BuildMotionData(MotionDataBuilderContext& context);
        };
    } // namespace Pipeline
} // namespace EMotionFX