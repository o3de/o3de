/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomLyIntegration/CommonFeatures/Debug/RayTracingDebugComponentConfig.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <Debug/RayTracingDebugComponentController.h>

namespace AZ::Render
{
    class RayTracingDebugComponent final
        : public AzFramework::Components::ComponentAdapter<RayTracingDebugComponentController, RayTracingDebugComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<RayTracingDebugComponentController, RayTracingDebugComponentConfig>;
        AZ_COMPONENT(RayTracingDebugComponent, "{143A2F13-8BE7-4F7F-BD23-4B5B8DCD83CA}", BaseClass);

        RayTracingDebugComponent() = default;
        RayTracingDebugComponent(const RayTracingDebugComponentConfig& config);

        static void Reflect(ReflectContext* context);
    };
} // namespace AZ::Render
