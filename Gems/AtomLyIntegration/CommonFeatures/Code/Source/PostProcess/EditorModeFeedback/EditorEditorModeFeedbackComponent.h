/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <PostProcess/EditorModeFeedback/EditorModeFeedbackComponent.h>

namespace AZ
{
    namespace Render
    {
        class EditorEditorModeFeedbackComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<EditorModeFeedbackComponentController, EditorModeFeedbackComponent, EditorModeFeedbackComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<EditorModeFeedbackComponentController, EditorModeFeedbackComponent, EditorModeFeedbackComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorEditorModeFeedbackComponent, "{4B044C5D-573A-4CCA-BF66-856C91F7B72F}", BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorEditorModeFeedbackComponent() = default;
            EditorEditorModeFeedbackComponent(const EditorModeFeedbackComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };
    } // namespace Render
} // namespace AZ
