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
#include <PostProcess/MotionBlur/MotionBlurComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace MotionBlur
        {
            static constexpr const char* const EditorMotionBlurComponentTypeId = "{ACA14BD9-BB53-4FEB-9E07-FDC0C9FE64FE}";
        }

        class EditorMotionBlurComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<MotionBlurComponentController, MotionBlurComponent, MotionBlurComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<MotionBlurComponentController, MotionBlurComponent, MotionBlurComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorMotionBlurComponent, MotionBlur::EditorMotionBlurComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorMotionBlurComponent() = default;
            EditorMotionBlurComponent(const MotionBlurComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
