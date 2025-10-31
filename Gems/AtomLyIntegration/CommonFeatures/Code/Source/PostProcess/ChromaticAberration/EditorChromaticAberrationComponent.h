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
#include <PostProcess/ChromaticAberration/ChromaticAberrationComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace ChromaticAberration
        {
            inline constexpr AZ::TypeId EditorChromaticAberrationComponentTypeId{ "{2F6115F3-C6C4-47ED-8F06-11006F190A68}" };
        }

        class EditorChromaticAberrationComponent final
            : public AzToolsFramework::Components::
                  EditorComponentAdapter<ChromaticAberrationComponentController, ChromaticAberrationComponent, ChromaticAberrationComponentConfig>
        {
        public:
            using BaseClass = AzToolsFramework::Components::
                EditorComponentAdapter<ChromaticAberrationComponentController, ChromaticAberrationComponent, ChromaticAberrationComponentConfig>;
            AZ_EDITOR_COMPONENT(AZ::Render::EditorChromaticAberrationComponent, ChromaticAberration::EditorChromaticAberrationComponentTypeId, BaseClass);

            static void Reflect(AZ::ReflectContext* context);

            EditorChromaticAberrationComponent() = default;
            EditorChromaticAberrationComponent(const ChromaticAberrationComponentConfig& config);

            //! EditorRenderComponentAdapter overrides...
            AZ::u32 OnConfigurationChanged() override;
        };

    } // namespace Render
} // namespace AZ
