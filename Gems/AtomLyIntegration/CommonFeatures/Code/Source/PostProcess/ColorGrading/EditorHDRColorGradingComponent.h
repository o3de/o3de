/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/ColorGrading/HDRColorGradingComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorHDRColorGradingComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<HDRColorGradingComponentController, HDRColorGradingComponent, HDRColorGradingComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<HDRColorGradingComponentController, HDRColorGradingComponent, HDRColorGradingComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorHDRColorGradingComponent, "{C1FAB0B1-5847-4533-B08E-7314AC807B8E}", BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorHDRColorGradingComponent() = default;
            EditorHDRColorGradingComponent(const HDRColorGradingComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };
    } // namespace Render
} // namespace AZ
