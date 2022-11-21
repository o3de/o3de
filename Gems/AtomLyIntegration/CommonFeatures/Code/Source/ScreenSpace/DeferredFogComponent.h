/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            inline constexpr AZ::TypeId DeferredFogComponentTypeId{ "{9492DC07-B3F7-4DF2-88FA-E4EEF1DD98E3}" };
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
