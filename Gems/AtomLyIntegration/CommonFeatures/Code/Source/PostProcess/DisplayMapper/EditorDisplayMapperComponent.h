/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/DisplayMapper/DisplayMapperComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorDisplayMapperComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<DisplayMapperComponentController, DisplayMapperComponent, DisplayMapperComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<DisplayMapperComponentController, DisplayMapperComponent, DisplayMapperComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorDisplayMapperComponent, EditorDisplayMapperComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorDisplayMapperComponent() = default;
            EditorDisplayMapperComponent(const DisplayMapperComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
