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

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/ScreenSpace/DeferredFogComponentConfig.h>
#include <ScreenSpace/DeferredFogComponentController.h>

namespace AZ
{
    namespace Render
    {
        namespace DeferredFog
        {
            static constexpr const char* const DeferredFogComponentTypeId = "{9492DC07-B3F7-4DF2-88FA-E4EEF1DD98E3}";
        }

        class DeferredFogComponent final
            : public AzFramework::Components::ComponentAdapter<DeferredFogComponentController, DeferredFogComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<DeferredFogComponentController, DeferredFogComponentConfig>;
            AZ_COMPONENT(AZ::Render::DeferredFogComponent, DeferredFog::DeferredFogComponentTypeId, BaseClass);

            DeferredFogComponent() = default;
            DeferredFogComponent(const DeferredFogComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
