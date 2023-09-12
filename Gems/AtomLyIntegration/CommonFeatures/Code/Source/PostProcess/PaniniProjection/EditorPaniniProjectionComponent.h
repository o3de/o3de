/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldConstants.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/PaniniProjection/PaniniProjectionComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace PaniniProjection
        {
            static constexpr const char* const EditorPaniniProjectionComponentTypeId = "{213CFFC8-E9E6-46EA-9DBE-B779F0B2A823}";
        }

        class EditorPaniniProjectionComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<PaniniProjectionComponentController, PaniniProjectionComponent, PaniniProjectionComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<PaniniProjectionComponentController, PaniniProjectionComponent, PaniniProjectionComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorPaniniProjectionComponent, PaniniProjection::EditorPaniniProjectionComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorPaniniProjectionComponent() = default;
            EditorPaniniProjectionComponent(const PaniniProjectionComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
