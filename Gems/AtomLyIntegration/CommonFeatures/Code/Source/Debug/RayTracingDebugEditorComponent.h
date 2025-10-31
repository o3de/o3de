/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomLyIntegration/CommonFeatures/Debug/RayTracingDebugComponentConfig.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Debug/RayTracingDebugComponent.h>
#include <Debug/RayTracingDebugComponentController.h>

namespace AZ::Render
{
    class RayTracingDebugEditorComponent final
        : public AzToolsFramework::Components::
              EditorComponentAdapter<RayTracingDebugComponentController, RayTracingDebugComponent, RayTracingDebugComponentConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::
            EditorComponentAdapter<RayTracingDebugComponentController, RayTracingDebugComponent, RayTracingDebugComponentConfig>;
        AZ_EDITOR_COMPONENT(RayTracingDebugEditorComponent, "{352A6033-4127-4A8D-BE8A-3FA1267B02EB}", BaseClass);

        static void Reflect(ReflectContext* context);

        // EditorComponentAdapter overrides
        u32 OnConfigurationChanged() override;
    };
} // namespace AZ::Render
