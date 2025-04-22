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
#include <AtomLyIntegration/CommonFeatures/Debug/RenderDebugComponentConfig.h>
#include <Atom/Feature/Debug/RenderDebugConstants.h>
#include <Debug/RenderDebugComponentController.h>

namespace AZ::Render
{
    namespace RenderDebug
    {
        inline constexpr AZ::TypeId RenderDebugComponentTypeId{ "{98A72F68-3DA3-451A-BC79-707370EE4AC0}" };
    }

    class RenderDebugComponent final
        : public AzFramework::Components::ComponentAdapter<RenderDebugComponentController, RenderDebugComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<RenderDebugComponentController, RenderDebugComponentConfig>;
        AZ_COMPONENT(AZ::Render::RenderDebugComponent, RenderDebug::RenderDebugComponentTypeId , BaseClass);

        RenderDebugComponent() = default;
        RenderDebugComponent(const RenderDebugComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };

}
