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

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(AZ::SceneAPI::Events::ExportEventContext& context) const;
        };
    } // namespace Pipeline
} // namespace EmotionFX