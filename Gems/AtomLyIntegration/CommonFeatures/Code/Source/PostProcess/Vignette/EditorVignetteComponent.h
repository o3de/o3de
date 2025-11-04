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
#include <PostProcess/Vignette/VignetteComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace Vignette
        {
            static constexpr const char* const EditorVignetteComponentTypeId = "{8E362EA9-76D8-4EBC-B73D-400DF3DF8B4A}";
        }

        class EditorVignetteComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<VignetteComponentController, VignetteComponent, VignetteComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<VignetteComponentController, VignetteComponent, VignetteComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorVignetteComponent, Vignette::EditorVignetteComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorVignetteComponent() = default;
            EditorVignetteComponent(const VignetteComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
