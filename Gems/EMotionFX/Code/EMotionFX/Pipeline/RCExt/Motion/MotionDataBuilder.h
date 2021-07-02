/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
