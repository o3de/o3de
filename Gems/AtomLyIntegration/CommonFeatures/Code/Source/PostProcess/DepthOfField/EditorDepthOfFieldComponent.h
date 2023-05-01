/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <PostProcess/DepthOfField/DepthOfFieldComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace DepthOfField
        {
            inline constexpr AZ::TypeId EditorDepthOfFieldComponentTypeId{ "{E9B85017-18F3-4CD6-9EEC-221B0E6B0619}" };
        }

        class EditorDepthOfFieldComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<DepthOfFieldComponentController, DepthOfFieldComponent, DepthOfFieldComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<DepthOfFieldComponentController, DepthOfFieldComponent, DepthOfFieldComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDepthOfFieldComponent, DepthOfField::EditorDepthOfFieldComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorDepthOfFieldComponent() = default;
            EditorDepthOfFieldComponent(const DepthOfFieldComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
