/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/ExposureControl/ExposureControlComponentConstants.h>
#include <PostProcess/LookModification/LookModificationComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorLookModificationComponent final
            : public AzToolsFramework::Components::EditorComponentAdapter<LookModificationComponentController, LookModificationComponent, LookModificationComponentConfig>
        {
        public:

            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<LookModificationComponentController, LookModificationComponent, LookModificationComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorLookModificationComponent, EditorLookModificationComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorLookModificationComponent() = default;
            EditorLookModificationComponent(const LookModificationComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
