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
        class ActorExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(ActorExporter, "{51462BD8-D376-438E-B1B6-8978D07A7E1C}", AZ::SceneAPI::SceneCore::ExportingComponent);

            ActorExporter();
            ~ActorExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            uint8_t GetPriority() const override;

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(AZ::SceneAPI::Events::ExportEventContext& context) const;
        };
    } // namespace Pipeline
} // namespace EmotionFX
