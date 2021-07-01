/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            class ExportEventContext;
        }
    }
}

namespace EMotionFX
{
    namespace Pipeline
    {
        class MotionExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(MotionExporter, "{A4F826D8-D710-4DF2-B660-9ED010072C1B}", AZ::SceneAPI::SceneCore::ExportingComponent);

            explicit MotionExporter();
            ~MotionExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(AZ::SceneAPI::Events::ExportEventContext& context) const;
        };
    } // namespace Pipeline
} // namespace EMotionFX
