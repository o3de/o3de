/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <PostProcess/ExposureControl/ExposureControlComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorExposureControlComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<ExposureControlComponentController, ExposureControlComponent, ExposureControlComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<ExposureControlComponentController, ExposureControlComponent, ExposureControlComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorExposureControlComponent, EditorExposureControlComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorExposureControlComponent() = default;
            EditorExposureControlComponent(const ExposureControlComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
