/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <PostProcess/EditorModeFeedback/EditorModeFeedbackComponentController.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/EditorModeFeedback/EditorModeFeedbackComponentConfig.h>

namespace AZ
{
    namespace Render
    {
        class EditorModeFeedbackComponent final
            : public AzFramework::Components::ComponentAdapter<EditorModeFeedbackComponentController, EditorModeFeedbackComponentConfig>
        {
        public:
            using BaseClass = AzFramework::Components::ComponentAdapter<EditorModeFeedbackComponentController, EditorModeFeedbackComponentConfig>;
            AZ_COMPONENT(AZ::Render::EditorModeFeedbackComponent, "{F7B22996-98DF-469F-AD32-277AFBC47B56}", BaseClass);

            EditorModeFeedbackComponent() = default;
            EditorModeFeedbackComponent(const EditorModeFeedbackComponentConfig& config);

            static void Reflect(AZ::ReflectContext* context);
        };
    } // namespace Render
} // namespace AZ
