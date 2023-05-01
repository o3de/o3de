/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Atom/Feature/Debug/RenderDebugConstants.h>
#include <Debug/RenderDebugComponent.h>

namespace AZ::Render
{
    namespace RenderDebug
    {
        inline constexpr AZ::TypeId RenderDebugEditorComponentTypeId{ "{235031F8-2AAD-442D-AB4D-F9B5B8337DCD}" };
    }

    class RenderDebugEditorComponent final
        : public AzToolsFramework::Components::EditorComponentAdapter<RenderDebugComponentController, RenderDebugComponent, RenderDebugComponentConfig>
    {
    public:

        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<RenderDebugComponentController, RenderDebugComponent, RenderDebugComponentConfig>;
        AZ_EDITOR_COMPONENT(AZ::Render::RenderDebugEditorComponent, RenderDebug::RenderDebugEditorComponentTypeId, BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        RenderDebugEditorComponent() = default;
        RenderDebugEditorComponent(const RenderDebugComponentConfig& config);

        //! EditorRenderComponentAdapter overrides...
        AZ::u32 OnConfigurationChanged() override;
    };

}
